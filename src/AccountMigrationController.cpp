// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AccountMigrationController.h"

// Qt
#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrl>
// QXmpp
#include <QXmppMovedManager.h>
#include <QXmppPubSubManager.h>
// Kaidan
#include "AccountDb.h"
#include "AccountMigrationData.h"
#include "KaidanCoreLog.h"
#include "MainController.h"
#include "RosterDb.h"
#include "RosterModel.h"

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

    connect(migrationManager, &QXmppAccountMigrationManager::errorOccurred, this, [this, promise](const QXmppError &error) mutable {
        informUser(error.description);
        reportFinishedResult(*promise, false);
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

    newAccount->clientController()->accountMigrationManager()->importData(m_exportData).then(this, [this, promise, newAccount](auto &&result) mutable {
        if (const auto accountFilePath = diskAccountFilePath(); const auto error = std::get_if<QXmppError>(&result)) {
            saveAccountDataToDisk(accountFilePath, m_exportData);
            informUser(error->description);
            promise->finish();
        } else {
            if (QFile::exists(accountFilePath)) {
                QFile::remove(accountFilePath);
            }

            if (const auto oldClientSettings = m_exportData.extension<ClientSettings>(); oldClientSettings) {
                importClientSettings(newAccount, *oldClientSettings)
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
                                                informUser(tr("Account '%1' migrated to '%2'", "%1 and %2 are account display names")
                                                               .arg(m_oldAccount->settings()->displayName())
                                                               .arg(newAccount->settings()->displayName()));
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

QFuture<void> AccountMigrationController::importClientSettings(Account *newAccount, const ClientSettings &oldClientSettings)
{
    auto promise = std::make_shared<QPromise<void>>();
    const auto newAccountJid = newAccount->settings()->jid();

    AccountDb::instance()
        ->updateAccount(newAccountJid,
                        [settings = oldClientSettings.oldConfiguration](AccountSettings::Data &account) {
                            if (settings.tlsErrorsIgnored) {
                                account.tlsErrorsIgnored = *settings.tlsErrorsIgnored;
                            }

                            if (settings.tlsRequirement) {
                                account.tlsRequirement = *settings.tlsRequirement;
                            }

                            if (settings.passwordVisibility) {
                                account.passwordVisibility = *settings.passwordVisibility;
                            }

                            if (settings.encryption) {
                                account.encryption = *settings.encryption;
                            }

                            if (settings.automaticMediaDownloadsRule) {
                                account.automaticMediaDownloadsRule = *settings.automaticMediaDownloadsRule;
                            }

                            if (settings.name) {
                                account.name = *settings.name;
                            }

                            if (settings.contactNotificationRule) {
                                account.contactNotificationRule = *settings.contactNotificationRule;
                            }

                            if (settings.groupChatNotificationRule) {
                                account.groupChatNotificationRule = *settings.groupChatNotificationRule;
                            }

                            if (settings.geoLocationMapPreviewEnabled) {
                                account.geoLocationMapPreviewEnabled = *settings.geoLocationMapPreviewEnabled;
                            }

                            if (settings.geoLocationMapService) {
                                account.geoLocationMapService = *settings.geoLocationMapService;
                            }
                        })
        .then(this, [promise, newAccountJid, oldClientSettings, this]() mutable {
            QList<QFuture<void>> futures;
            const auto oldRosterItemSettings = oldClientSettings.roster;

            for (const ClientRosterItemSettings &itemSettings : oldRosterItemSettings) {
                auto updateRosterItem = [newAccountJid, itemSettings]() {
                    return RosterDb::instance()->updateItem(newAccountJid, itemSettings.jid, [itemSettings](RosterItem &item) {
                        item.encryption = itemSettings.encryption.value_or(item.encryption);
                        item.notificationRule = itemSettings.notificationRule.value_or(item.notificationRule);
                        item.chatStateSendingEnabled = itemSettings.chatStateSendingEnabled.value_or(item.chatStateSendingEnabled);
                        item.readMarkerSendingEnabled = itemSettings.readMarkerSendingEnabled.value_or(item.readMarkerSendingEnabled);
                        item.pinningPosition = itemSettings.pinningPosition.value_or(item.pinningPosition);
                        item.automaticMediaDownloadsRule = itemSettings.automaticMediaDownloadsRule.value_or(item.automaticMediaDownloadsRule);
                    });
                };

                if (RosterModel::instance()->hasItem(newAccountJid, itemSettings.jid)) {
                    futures.append(updateRosterItem());
                } else {
                    auto updateRosterItemOnceAdded = [this, newAccountJid, itemSettings, updateRosterItem]() {
                        auto promise = std::make_shared<QPromise<void>>();
                        promise->start();
                        auto *context = new QObject(this);

                        connect(RosterDb::instance(),
                                &RosterDb::itemAdded,
                                context,
                                [context, promise, newAccountJid, jid = itemSettings.jid, updateRosterItem](const RosterItem &rosterItem) mutable {
                                    if (rosterItem.accountJid == newAccountJid && rosterItem.jid == jid) {
                                        updateRosterItem().then(context, [context, promise]() mutable {
                                            context->deleteLater();
                                            promise->finish();
                                        });
                                    }
                                });

                        return promise->future();
                    };

                    futures.append(updateRosterItemOnceAdded());
                }
            }

            joinVoidFutures(std::move(futures)).then([promise]() mutable {
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
