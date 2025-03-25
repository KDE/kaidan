// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Mathis Brüchert <mbblp@protonmail.ch>
// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AccountController.h"

// Qt
#include <QMutexLocker>
#include <QSettings>
#include <QStringBuilder>
// QXmpp
#include <QXmppCredentials.h>
#include <QXmppSasl2UserAgent.h>
#include <QXmppUtils.h>
// Kaidan
#include "AccountDb.h"
#include "EncryptionController.h"
#include "Globals.h"
#include "Kaidan.h"
#include "MessageDb.h"
#include "RegistrationController.h"
#include "RosterDb.h"
#include "ServerFeaturesCache.h"
#include "Settings.h"
#include "SystemUtils.h"
#include "VCardCache.h"

#define UPDATE_ACCOUNT(PROPERTY)                                                                                                                               \
    {                                                                                                                                                          \
        QMutexLocker locker(&m_mutex);                                                                                                                         \
        if (m_tmpAccount) {                                                                                                                                    \
            m_tmpAccount->PROPERTY = PROPERTY;                                                                                                                 \
            locker.unlock();                                                                                                                                   \
            Q_EMIT accountChanged();                                                                                                                           \
            return;                                                                                                                                            \
        }                                                                                                                                                      \
    }                                                                                                                                                          \
    AccountDb::instance()->updateAccount(AccountController::account().jid, [PROPERTY](Account &account) {                                                      \
        account.PROPERTY = PROPERTY;                                                                                                                           \
    });

AccountController *AccountController::s_instance = nullptr;

AccountController *AccountController::instance()
{
    return s_instance;
}

AccountController::AccountController(Settings *settings, VCardCache *cache, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_clientWorker(Kaidan::instance()->client())
    , m_client(m_clientWorker->xmppClient())
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    connect(AccountDb::instance(), &AccountDb::accountUpdated, this, [this](const Account &account) {
        {
            QMutexLocker locker(&m_mutex);

            // Normally, AccountDb::accountUpdated() is only triggered for accounts already stored
            // in the database.
            // In that case, there would not be a temporary account.
            // If a new account is stored, a temporary account may still exist, though.
            // accountChanged() must not be emitted in that case.
            if (m_tmpAccount) {
                Q_ASSERT(m_tmpAccount->jid == account.jid);
                m_tmpAccount = account;
                return;
            } else {
                Q_ASSERT(m_account.jid == account.jid);
                m_account = account;
            }
        }

        Q_EMIT accountChanged();
    });

    connect(cache, &VCardCache::vCardChanged, this, [this](const QString &jid) {
        const auto currentJid = account().jid;

        if (currentJid == jid) {
            if (const auto vCard = Kaidan::instance()->vCardCache()->vCard(jid)) {
                const auto name = vCard->nickName();
                UPDATE_ACCOUNT(name);
            }
        }
    });
}

Account AccountController::account() const
{
    QMutexLocker locker(&m_mutex);
    return m_tmpAccount ? *m_tmpAccount : m_account;
}

bool AccountController::hasNewAccount() const
{
    QMutexLocker locker(&m_mutex);
    return m_tmpAccount.has_value();
}

void AccountController::setAuthOnline(bool online)
{
    UPDATE_ACCOUNT(online);
}

void AccountController::setAuthJidResourcePrefix(const QString &resourcePrefix)
{
    UPDATE_ACCOUNT(resourcePrefix);
}

void AccountController::setAuthPassword(const QString &password)
{
    UPDATE_ACCOUNT(password);
}

void AccountController::setAuthCredentials(const QXmppCredentials &credentials)
{
    UPDATE_ACCOUNT(credentials);
}

void AccountController::setAuthHost(const QString &host)
{
    UPDATE_ACCOUNT(host);
}

void AccountController::setAuthPort(quint16 port)
{
    UPDATE_ACCOUNT(port);
}

void AccountController::setAuthTlsErrorsIgnored(bool tlsErrorsIgnored)
{
    UPDATE_ACCOUNT(tlsErrorsIgnored);
}

void AccountController::setAuthTlsRequirement(QXmppConfiguration::StreamSecurityMode tlsRequirement)
{
    UPDATE_ACCOUNT(tlsRequirement);
}

void AccountController::setAuthPasswordVisibility(Kaidan::PasswordVisibility passwordVisibility)
{
    UPDATE_ACCOUNT(passwordVisibility);
}

void AccountController::setUserAgentDeviceId(const QUuid &userAgentDeviceId)
{
    UPDATE_ACCOUNT(userAgentDeviceId);
}

void AccountController::setEncryption(Encryption::Enum encryption)
{
    UPDATE_ACCOUNT(encryption);
}

void AccountController::setAutomaticMediaDownloadsRule(Account::AutomaticMediaDownloadsRule automaticMediaDownloadsRule)
{
    UPDATE_ACCOUNT(automaticMediaDownloadsRule);
}

void AccountController::setContactNotificationRule(const QString &jid, Account::ContactNotificationRule contactNotificationRule)
{
    Q_UNUSED(jid)
    UPDATE_ACCOUNT(contactNotificationRule);
}

void AccountController::setGroupChatNotificationRule(const QString &jid, Account::GroupChatNotificationRule groupChatNotificationRule)
{
    Q_UNUSED(jid)
    UPDATE_ACCOUNT(groupChatNotificationRule);
}

void AccountController::setGeoLocationMapPreviewEnabled(const QString &jid, bool geoLocationMapPreviewEnabled)
{
    Q_UNUSED(jid)
    UPDATE_ACCOUNT(geoLocationMapPreviewEnabled);
}

void AccountController::setGeoLocationMapService(const QString &jid, Account::GeoLocationMapService geoLocationMapService)
{
    Q_UNUSED(jid)
    UPDATE_ACCOUNT(geoLocationMapService);
}

void AccountController::setNewAccount(const QString &jid, const QString &password, const QString &host, quint16 port)
{
    if (AccountController::account().jid != jid) {
        {
            QMutexLocker locker(&m_mutex);

            m_tmpAccount = [&]() -> std::optional<Account> {
                if (jid.isEmpty()) {
                    return {};
                }

                Account account;
                account.jid = jid;
                account.password = password;
                account.host = host;
                account.port = port;
                return account;
            }();

            if (jid.isEmpty() && !m_tmpAccount) {
                m_account = {};
            }
        }

        Q_EMIT accountChanged();
    }
}

void AccountController::setNewAccountJid(const QString &jid)
{
    setNewAccount(jid, {});
}

void AccountController::setNewAccountHost(const QString &host, quint16 port)
{
    {
        QMutexLocker locker(&m_mutex);

        if (!m_tmpAccount) {
            return;
        }

        m_tmpAccount->host = host;
        m_tmpAccount->port = port;
    }

    Q_EMIT accountChanged();
}

void AccountController::resetNewAccount()
{
    {
        QMutexLocker locker(&m_mutex);

        if (!m_tmpAccount) {
            return;
        }

        m_tmpAccount.reset();
    }

    Q_EMIT accountChanged();
}

quint16 AccountController::portAutodetect() const
{
    return PORT_AUTODETECT;
}

void AccountController::resetCustomConnectionSettings()
{
    setNewAccountHost({}, PORT_AUTODETECT);
}

bool AccountController::hasEnoughCredentialsForLogin()
{
    const auto account = AccountController::account();
    return !(account.jid.isEmpty() || account.password.isEmpty());
}

QXmppSasl2UserAgent AccountController::userAgent()
{
    auto deviceId = account().userAgentDeviceId;
    if (deviceId.isNull()) {
        deviceId = QUuid::createUuid();
        setUserAgentDeviceId(deviceId);
    }
    return QXmppSasl2UserAgent(deviceId, QStringLiteral(APPLICATION_DISPLAY_NAME), SystemUtils::productName());
}

void AccountController::loadConnectionData()
{
    if (!hasEnoughCredentialsForLogin()) {
        AccountDb::instance()->lastAccount().then(this, [this](Account &&account) {
            const auto jid = account.jid;

            Kaidan::instance()->serverFeaturesCache()->setHttpUploadLimit(account.httpUploadLimit);

            {
                QMutexLocker locker(&m_mutex);

                Q_ASSERT(!m_tmpAccount);
                m_account = std::move(account);
            }

            Q_EMIT accountChanged();
            Q_EMIT connectionDataLoaded();
        });
    }
}

void AccountController::storeAccount()
{
    if (!hasNewAccount()) {
        return;
    }

    auto currentAccount = AccountController::account();
    const auto currentJid = currentAccount.jid;

    AccountDb::instance()->addAccount(currentJid).then(this, [this, currentJid, newAccount = std::move(currentAccount)]() {
        AccountDb::instance()
            ->updateAccount(currentJid,
                            [newAccount](Account &account) {
                                account = newAccount;
                            })
            .then(this, [this]() {
                {
                    QMutexLocker locker(&m_mutex);
                    m_account = *m_tmpAccount;
                    m_tmpAccount.reset();
                }

                Q_EMIT accountChanged();
            });
    });
}

void AccountController::deleteAccountFromClient()
{
    m_deletionStates = DeletionState::ToBeDeletedFromClient;

    // If the client is not yet disconnected, disconnect first and delete the account afterwards.
    // Otherwise, delete the account directly from the client.
    runOnThread(
        m_client,
        [this]() {
            return m_client->isAuthenticated();
        },
        this,
        [this](bool authenticated) {
            if (authenticated) {
                EncryptionController::instance()->reset().then(this, [this]() {
                    runOnThread(m_clientWorker, [this]() {
                        m_clientWorker->logOut();
                    });
                });
            } else {
                runOnThread(m_clientWorker, [this]() {
                    m_clientWorker->logIn();
                });
            }
        });
}

void AccountController::deleteAccountFromClientAndServer()
{
    m_deletionStates = DeletionState::ToBeDeletedFromClient | DeletionState::ToBeDeletedFromServer;

    // If the client is already connected, delete the account directly from the server.
    // Otherwise, connect first and delete the account afterwards.
    runOnThread(
        m_client,
        [this]() {
            return m_client->isAuthenticated();
        },
        this,
        [this](bool authenticated) {
            if (authenticated) {
                Kaidan::instance()->registrationController()->deleteAccount();
            } else {
                m_deletionStates |= DeletionState::ClientDisconnectedBeforeDeletionFromServer;

                runOnThread(m_clientWorker, [this]() {
                    m_clientWorker->logIn();
                });
            }
        });
}

void AccountController::handleAccountDeletedFromServer()
{
    m_deletionStates = DeletionState::DeletedFromServer;
}

void AccountController::handleAccountDeletionFromServerFailed(const QXmppStanza::Error &error)
{
    Q_EMIT accountDeletionFromClientAndServerFailed(error.text());

    if (m_deletionStates.testFlag(DeletionState::ClientDisconnectedBeforeDeletionFromServer)) {
        m_deletionStates = DeletionState::NotToBeDeleted;

        runOnThread(m_clientWorker, [this]() {
            m_clientWorker->logOut();
        });
    } else {
        m_deletionStates = DeletionState::NotToBeDeleted;
    }
}

bool AccountController::handleConnected()
{
    // If the account could not be deleted because the client was disconnected, delete it now.
    if (m_deletionStates.testFlag(DeletionState::ToBeDeletedFromClient) && m_deletionStates.testFlag(DeletionState::ToBeDeletedFromServer)) {
        Kaidan::instance()->registrationController()->deleteAccount();

        return true;
    } else if (m_deletionStates.testFlag(DeletionState::ToBeDeletedFromClient)) {
        deleteAccountFromClient();
        return true;
    }

    return false;
}

void AccountController::handleDisconnected()
{
    // Delete the account from the client if the account was deleted from the server or the client
    // was connected and had to disconnect first.
    if (m_deletionStates.testFlag(DeletionState::DeletedFromServer)) {
        EncryptionController::instance()->resetLocally().then(this, [this]() {
            removeAccount(account().jid);
        });
    } else if (m_deletionStates.testFlag(DeletionState::ToBeDeletedFromClient)) {
        removeAccount(account().jid);
    }

    m_deletionStates = DeletionState::NotToBeDeleted;
}

void AccountController::removeAccount(const QString &accountJid)
{
    MessageDb::instance()->removeAllMessagesFromAccount(accountJid);
    RosterDb::instance()->removeItems(accountJid);
    AccountDb::instance()->removeAccount(accountJid);

    m_settings->remove({
        QStringLiteral(KAIDAN_SETTINGS_FAVORITE_EMOJIS),
    });

    setNewAccount({}, {}, {}, PORT_AUTODETECT);

    Q_EMIT credentialsNeeded();
}

#include "moc_AccountController.cpp"
