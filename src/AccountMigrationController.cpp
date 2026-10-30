// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AccountMigrationController.h"

// std
#include <chrono>
// Qt
#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QHash>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
// QXmpp
#include <QXmppMovedManager.h>
#include <QXmppPubSubManager.h>
// Kaidan
#include "AccountMigrationData.h"
#include "FutureUtils.h"
#include "KaidanCoreLog.h"
#include "MainController.h"
#include "RosterDb.h"
#include "RosterModel.h"

using namespace std::chrono_literals;

// How long to wait for contacts missing from the new account's roster to be added.
static constexpr auto PENDING_ROSTER_ITEM_SETTINGS_TIMEOUT = 2min;

AccountMigrationController::AccountMigrationController(QObject *parent)
    : QObject(parent)
{
    constexpr const auto parseData = &ClientSettings::fromDom;
    const auto serializeData = [](const ClientSettings &data, QXmlStreamWriter &writer) {
        data.toXml(writer);
    };

    QXmppExportData::registerExtension<ClientSettings, parseData, serializeData>(s_client_settings, s_kaidan_export_ns);
}

AccountMigrationController::~AccountMigrationController() = default;

QFuture<bool> AccountMigrationController::startMigration(Account *oldAccount)
{
    auto promise = std::make_shared<QPromise<bool>>();
    promise->start();

    m_oldAccount = oldAccount;
    auto *migrationManager = m_oldAccount->clientController()->accountMigrationManager();

    // QXmpp reports errors this way that do not abort the migration (e.g., for MIX channels).
    connect(migrationManager, &QXmppAccountMigrationManager::errorOccurred, this, [](const QXmppError &error) {
        informUser(error.description);
    });

    if (m_oldAccount->connection()->state() == Enums::ConnectionState::StateConnected) {
        if (const auto accountFilePath = diskAccountFilePath(); QFile::exists(accountFilePath)) {
            if (restoreAccountDataFromDisk(accountFilePath, m_exportData)) {
                reportFinishedResult(*promise, true);
            } else {
                informUser(tr("Account could not be migrated: Could not load exported account data"));
                reportFinishedResult(*promise, false);
            }
        } else {
            migrationManager->exportData().then(this, [this, promise](QXmppAccountMigrationManager::Result<QXmppExportData> &&result) mutable {
                if (const auto error = std::get_if<QXmppError>(&result)) {
                    informUser(error->description);
                    reportFinishedResult(*promise, false);
                } else {
                    m_exportData = std::move(std::get<QXmppExportData>(std::move(result)));
                    m_exportData.setExtension(exportClientSettings());
                    reportFinishedResult(*promise, true);
                }
            });
        }

    } else {
        informUser(tr("Account could not be migrated: You need to be connected"));
        reportFinishedResult(*promise, false);
    }

    return promise->future();
}

QFuture<void> AccountMigrationController::finalizeMigration(Account *newAccount)
{
    auto promise = std::make_shared<QPromise<void>>();
    promise->start();

    auto *migrationManager = newAccount->clientController()->accountMigrationManager();

    connect(migrationManager, &QXmppAccountMigrationManager::errorOccurred, this, [](const QXmppError &error) {
        informUser(error.description);
    });

    migrationManager->importData(m_exportData).then(this, [this, promise, newAccount](auto &&result) mutable {
        if (const auto accountFilePath = diskAccountFilePath(); const auto error = std::get_if<QXmppError>(&result)) {
            saveAccountDataToDisk(accountFilePath, m_exportData);
            informUser(error->description);
            promise->finish();
        } else {
            if (QFile::exists(accountFilePath)) {
                QFile::remove(accountFilePath);
            }

            if (const auto oldClientSettings = m_exportData.extension<ClientSettings>(); oldClientSettings) {
                importClientSettings(newAccount->settings(), *oldClientSettings)
                    .then([this, promise, newAccount, oldContacts = oldClientSettings->rosterContacts()]() mutable {
                        m_oldAccount->clientController()
                            ->movedManager()
                            ->publishStatement(newAccount->settings()->jid())
                            .then(this, [this, promise, newAccount, oldContacts](auto &&result) mutable {
                                if (const auto error = std::get_if<QXmppError>(&result)) {
                                    informUser(error->description);
                                    promise->finish();
                                } else {
                                    notifyContacts(newAccount, oldContacts)
                                        .then([this, promise, newAccount](QXmppAccountMigrationManager::Result<> result) mutable {
                                            if (const auto error = std::get_if<QXmppError>(&result)) {
                                                informUser(error->description);
                                            } else {
                                                informUser(tr("Account %1 (%2) migrated to %3 (%4)",
                                                              "%1 and %3 are account display names; %2 and %4 are chat addresses")
                                                               .arg(m_oldAccount->settings()->displayName())
                                                               .arg(m_oldAccount->settings()->jid())
                                                               .arg(newAccount->settings()->displayName())
                                                               .arg(newAccount->settings()->jid()));
                                            }

                                            promise->finish();
                                        });
                                }
                            });
                    });
            } else {
                promise->finish();
            }
        }
    });

    return promise->future();
}

void AccountMigrationController::informUser(const QString &notification)
{
    Q_EMIT MainController::instance()->passiveNotificationRequested(notification);
}

QString AccountMigrationController::diskAccountFilePath() const
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
        .absoluteFilePath(QStringLiteral("%1.xml").arg(m_oldAccount->settings()->jid()));
}

bool AccountMigrationController::saveAccountDataToDisk(const QString &filePath, const QXmppExportData &data)
{
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCDebug(KAIDAN_CORE_LOG,
                "Account could not be migrated: Could not open account file '%ls' for writing: %ls",
                qUtf16Printable(filePath),
                qUtf16Printable(file.errorString()));
        return false;
    }

    QXmlStreamWriter writer(&file);

    data.toXml(&writer, QXmppExportData::Format::Xep0227);

    if (writer.hasError()) {
        file.remove();
        qCDebug(KAIDAN_CORE_LOG,
                "Account could not be migrated: Could not write account data to file '%ls': %ls",
                qUtf16Printable(filePath),
                qUtf16Printable(file.errorString()));
        return false;
    }

    return true;
}

bool AccountMigrationController::restoreAccountDataFromDisk(const QString &filePath, QXmppExportData &data)
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        qCDebug(KAIDAN_CORE_LOG,
                "Account could not be migrated: Could not open account file '%ls' for reading: %ls",
                qUtf16Printable(filePath),
                qUtf16Printable(file.errorString()));
        return false;
    }

    QDomDocument document;

    if (const auto res = document.setContent(&file, QDomDocument::ParseOption::UseNamespaceProcessing); !res) {
        qCDebug(KAIDAN_CORE_LOG,
                "Account could not be migrated: Could not parse account data from '%ls': %ls",
                qUtf16Printable(filePath),
                qUtf16Printable(res.errorMessage));
        return false;
    }

    const auto result = QXmppExportData::fromDom(document.documentElement());

    if (const auto error = std::get_if<QXmppError>(&result)) {
        qCDebug(KAIDAN_CORE_LOG,
                "Account could not be migrated: Could not read account data from file '%ls': %ls",
                qUtf16Printable(filePath),
                qUtf16Printable(error->description));
        return false;
    }

    data = std::move(std::get<QXmppExportData>(result));

    return true;
}

ClientSettings AccountMigrationController::exportClientSettings()
{
    auto *accountSettings = m_oldAccount->settings();
    return ClientSettings(accountSettings, RosterModel::instance()->items(accountSettings->jid()));
}

static QFuture<void> updateRosterItemSettings(const QString &accountJid, const ClientRosterItemSettings &itemSettings)
{
    return RosterDb::instance()->updateItem(accountJid, itemSettings.jid, [itemSettings](RosterItem &item) {
        item.encryption = itemSettings.encryption.value_or(item.encryption);
        item.notificationRule = itemSettings.notificationRule.value_or(item.notificationRule);
        item.chatStateSendingEnabled = itemSettings.chatStateSendingEnabled.value_or(item.chatStateSendingEnabled);
        item.readMarkerSendingEnabled = itemSettings.readMarkerSendingEnabled.value_or(item.readMarkerSendingEnabled);
        item.pinningPosition = itemSettings.pinningPosition.value_or(item.pinningPosition);
        item.automaticMediaDownloadsRule = itemSettings.automaticMediaDownloadsRule.value_or(item.automaticMediaDownloadsRule);
    });
}

// Applies the settings to contacts once they are added to the roster later.
// The migration must not wait for that since some contacts may never be added (e.g., group chats
// that are not migrated).
static void updateRosterItemSettingsOnceAdded(QObject *context, const QString &accountJid, const QList<ClientRosterItemSettings> &roster)
{
    auto pendingSettings = std::make_shared<QHash<QString, ClientRosterItemSettings>>();

    for (const auto &itemSettings : roster) {
        pendingSettings->insert(itemSettings.jid, itemSettings);
    }

    auto *watcher = new QObject(context);

    QObject::connect(RosterDb::instance(), &RosterDb::itemAdded, watcher, [watcher, accountJid, pendingSettings](const RosterItem &item) {
        if (item.accountJid != accountJid) {
            return;
        }

        if (const auto itr = pendingSettings->constFind(item.jid); itr != pendingSettings->cend()) {
            updateRosterItemSettings(accountJid, *itr);
            pendingSettings->erase(itr);

            if (pendingSettings->isEmpty()) {
                watcher->deleteLater();
            }
        }
    });

    QTimer::singleShot(PENDING_ROSTER_ITEM_SETTINGS_TIMEOUT, watcher, &QObject::deleteLater);
}

QFuture<void> AccountMigrationController::importClientSettings(AccountSettings *accountSettings, const ClientSettings &oldClientSettings)
{
    const auto &settings = oldClientSettings.oldConfiguration;

    // Apply the settings to the account in use so that they take effect immediately and are not
    // overwritten by its outdated values later.
    if (settings.tlsErrorsIgnored) {
        accountSettings->setTlsErrorsIgnored(*settings.tlsErrorsIgnored);
    }

    if (settings.tlsRequirement) {
        accountSettings->setTlsRequirement(*settings.tlsRequirement);
    }

    if (settings.passwordVisibility) {
        accountSettings->setPasswordVisibility(*settings.passwordVisibility);
    }

    if (settings.encryption) {
        accountSettings->setEncryption(*settings.encryption);
    }

    if (settings.automaticMediaDownloadsRule) {
        accountSettings->setAutomaticMediaDownloadsRule(*settings.automaticMediaDownloadsRule);
    }

    if (settings.name) {
        accountSettings->setName(*settings.name);
    }

    if (settings.contactNotificationRule) {
        accountSettings->setContactNotificationRule(*settings.contactNotificationRule);
    }

    if (settings.groupChatNotificationRule) {
        accountSettings->setGroupChatNotificationRule(*settings.groupChatNotificationRule);
    }

    if (settings.geoLocationMapPreviewEnabled) {
        accountSettings->setGeoLocationMapPreviewEnabled(*settings.geoLocationMapPreviewEnabled);
    }

    if (settings.geoLocationMapService) {
        accountSettings->setGeoLocationMapService(*settings.geoLocationMapService);
    }

    auto promise = std::make_shared<QPromise<void>>();
    promise->start();

    // Store the settings that are not stored by their setters (e.g., the name).
    accountSettings->storeTemporaryData().then(
        accountSettings,
        [promise, accountSettings, accountJid = accountSettings->jid(), roster = oldClientSettings.roster]() mutable {
            // Watch before updating so that no contact added in between is missed.
            updateRosterItemSettingsOnceAdded(accountSettings, accountJid, roster);

            joinVoidFutures(transform<QList<QFuture<void>>>(roster, [accountJid](const ClientRosterItemSettings &itemSettings) {
                return updateRosterItemSettings(accountJid, itemSettings);
            })).then([promise]() mutable {
                promise->finish();
            });
        });

    return promise->future();
}

QFuture<QXmppAccountMigrationManager::Result<>> AccountMigrationController::notifyContacts(Account *newAccount, const QList<QString> &contactJids)
{
    if (contactJids.isEmpty()) {
        return QtFuture::makeReadyValueFuture<QXmppAccountMigrationManager::Result<>>({});
    }

    auto promise = std::make_shared<QPromise<QXmppAccountMigrationManager::Result<>>>();
    promise->start();

    auto counter = std::make_shared<int>(contactJids.size());

    for (const QString &contactJid : contactJids) {
        newAccount->clientController()
            ->movedManager()
            ->notifyContact(contactJid, m_exportData.accountJid(), false)
            .then(this, [promise, counter](auto &&result) mutable {
                if (promise->future().isFinished()) {
                    return;
                }

                if (const auto error = std::get_if<QXmppError>(&result)) {
                    promise->addResult(*error);
                    promise->finish();
                    return;
                }

                if ((--(*counter)) == 0) {
                    promise->addResult(QXmpp::Success());
                    promise->finish();
                }
            });
    }

    return promise->future();
}

#include "moc_AccountMigrationController.cpp"
