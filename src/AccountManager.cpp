// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Mathis Brüchert <mbblp@protonmail.ch>
// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AccountManager.h"

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
#include "RegistrationManager.h"
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
    await(AccountDb::instance()->updateAccount(AccountManager::account().jid,                                                                                  \
                                               [PROPERTY](Account &account) {                                                                                  \
                                                   account.PROPERTY = PROPERTY;                                                                                \
                                               }),                                                                                                             \
          this,                                                                                                                                                \
          [this, PROPERTY]() {                                                                                                                                 \
              {                                                                                                                                                \
                  QMutexLocker locker(&m_mutex);                                                                                                               \
                  m_account.PROPERTY = PROPERTY;                                                                                                               \
              }                                                                                                                                                \
              Q_EMIT accountChanged();                                                                                                                         \
          })

AccountManager *AccountManager::s_instance = nullptr;

AccountManager *AccountManager::instance()
{
    return s_instance;
}

AccountManager::AccountManager(Settings *settings, VCardCache *cache, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
{
    Q_ASSERT(!s_instance);
    s_instance = this;

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

Account AccountManager::account() const
{
    QMutexLocker locker(&m_mutex);
    return m_tmpAccount ? *m_tmpAccount : m_account;
}

bool AccountManager::hasNewAccount() const
{
    QMutexLocker locker(&m_mutex);
    return m_tmpAccount.has_value();
}

void AccountManager::setAuthOnline(bool online)
{
    UPDATE_ACCOUNT(online);
}

void AccountManager::setAuthJidResourcePrefix(const QString &resourcePrefix)
{
    UPDATE_ACCOUNT(resourcePrefix);
}

void AccountManager::setAuthPassword(const QString &password)
{
    UPDATE_ACCOUNT(password);
}

void AccountManager::setAuthCredentials(const QXmppCredentials &credentials)
{
    UPDATE_ACCOUNT(credentials);
}

void AccountManager::setAuthHost(const QString &host)
{
    UPDATE_ACCOUNT(host);
}

void AccountManager::setAuthPort(quint16 port)
{
    UPDATE_ACCOUNT(port);
}

void AccountManager::setAuthTlsErrorsIgnored(bool tlsErrorsIgnored)
{
    UPDATE_ACCOUNT(tlsErrorsIgnored);
}

void AccountManager::setAuthTlsRequirement(QXmppConfiguration::StreamSecurityMode tlsRequirement)
{
    UPDATE_ACCOUNT(tlsRequirement);
}

void AccountManager::setAuthPasswordVisibility(Kaidan::PasswordVisibility passwordVisibility)
{
    UPDATE_ACCOUNT(passwordVisibility);
}

void AccountManager::setUserAgentDeviceId(const QUuid &userAgentDeviceId)
{
    UPDATE_ACCOUNT(userAgentDeviceId);
}

void AccountManager::setEncryption(Encryption::Enum encryption)
{
    UPDATE_ACCOUNT(encryption);
}

void AccountManager::setAutomaticMediaDownloadsRule(Account::AutomaticMediaDownloadsRule automaticMediaDownloadsRule)
{
    UPDATE_ACCOUNT(automaticMediaDownloadsRule);
}

void AccountManager::setContactNotificationRule(const QString &jid, Account::ContactNotificationRule contactNotificationRule)
{
    Q_UNUSED(jid)
    UPDATE_ACCOUNT(contactNotificationRule);
}

void AccountManager::setGroupChatNotificationRule(const QString &jid, Account::GroupChatNotificationRule groupChatNotificationRule)
{
    Q_UNUSED(jid)
    UPDATE_ACCOUNT(groupChatNotificationRule);
}

void AccountManager::setGeoLocationMapPreviewEnabled(const QString &jid, bool geoLocationMapPreviewEnabled)
{
    Q_UNUSED(jid)
    UPDATE_ACCOUNT(geoLocationMapPreviewEnabled);
}

void AccountManager::setGeoLocationMapService(const QString &jid, Account::GeoLocationMapService geoLocationMapService)
{
    Q_UNUSED(jid)
    UPDATE_ACCOUNT(geoLocationMapService);
}

void AccountManager::setNewAccount(const QString &jid, const QString &password, const QString &host, quint16 port)
{
    if (AccountManager::account().jid != jid) {
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

void AccountManager::setNewAccountJid(const QString &jid)
{
    setNewAccount(jid, {});
}

void AccountManager::setNewAccountPassword(const QString &password)
{
    {
        QMutexLocker locker(&m_mutex);

        if (!m_tmpAccount) {
            return;
        }

        m_tmpAccount->password = password;
    }

    Q_EMIT accountChanged();
}

void AccountManager::setNewAccountHost(const QString &host, quint16 port)
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

void AccountManager::resetNewAccount()
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

quint16 AccountManager::portAutodetect() const
{
    return PORT_AUTODETECT;
}

void AccountManager::resetCustomConnectionSettings()
{
    setNewAccountHost({}, PORT_AUTODETECT);
}

bool AccountManager::hasEnoughCredentialsForLogin()
{
    const auto account = AccountManager::account();
    return !(account.jid.isEmpty() || account.password.isEmpty());
}

QXmppSasl2UserAgent AccountManager::userAgent()
{
    auto deviceId = account().userAgentDeviceId;
    if (deviceId.isNull()) {
        deviceId = QUuid::createUuid();
        setUserAgentDeviceId(deviceId);
    }
    return QXmppSasl2UserAgent(deviceId, QStringLiteral(APPLICATION_DISPLAY_NAME), SystemUtils::productName());
}

void AccountManager::loadConnectionData()
{
    if (!hasEnoughCredentialsForLogin()) {
        await(AccountDb::instance()->firstAccount(), this, [this](Account &&account) {
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

void AccountManager::storeAccount()
{
    if (!hasNewAccount()) {
        return;
    }

    auto currentAccount = AccountManager::account();
    const auto currentJid = currentAccount.jid;

    await(AccountDb::instance()->addAccount(currentJid), this, [this, currentJid, newAccount = std::move(currentAccount)]() {
        await(AccountDb::instance()->updateAccount(currentJid,
                                                   [&newAccount](Account &account) {
                                                       account = newAccount;
                                                   }),
              this,
              [this]() {
                  {
                      QMutexLocker locker(&m_mutex);
                      m_account = *m_tmpAccount;
                      m_tmpAccount.reset();
                  }

                  Q_EMIT accountChanged();
              });
    });
}

void AccountManager::deleteAccountFromClient()
{
    m_deletionStates = DeletionState::ToBeDeletedFromClient;

    // If the client is not yet disconnected, disconnect first and delete the account afterwards.
    // Otherwise, delete the account directly from the client.
    runOnThread(
        Kaidan::instance()->client(),
        []() {
            return Kaidan::instance()->client()->xmppClient()->isAuthenticated();
        },
        this,
        [this](bool authenticated) {
            if (authenticated) {
                await(EncryptionController::instance()->reset(), this, []() {
                    runOnThread(Kaidan::instance()->client(), []() {
                        Kaidan::instance()->client()->logOut();
                    });
                });
            } else {
                runOnThread(Kaidan::instance()->client(), []() {
                    Kaidan::instance()->client()->logIn();
                });
            }
        });
}

void AccountManager::deleteAccountFromClientAndServer()
{
    m_deletionStates = DeletionState::ToBeDeletedFromClient | DeletionState::ToBeDeletedFromServer;

    // If the client is already connected, delete the account directly from the server.
    // Otherwise, connect first and delete the account afterwards.
    runOnThread(
        Kaidan::instance()->client(),
        []() {
            return Kaidan::instance()->client()->xmppClient()->isAuthenticated();
        },
        this,
        [this](bool authenticated) {
            if (authenticated) {
                runOnThread(Kaidan::instance()->client(), []() {
                    Kaidan::instance()->client()->registrationManager()->deleteAccount();
                });
            } else {
                m_deletionStates |= DeletionState::ClientDisconnectedBeforeDeletionFromServer;

                runOnThread(Kaidan::instance()->client(), []() {
                    Kaidan::instance()->client()->logIn();
                });
            }
        });
}

void AccountManager::handleAccountDeletedFromServer()
{
    m_deletionStates = DeletionState::DeletedFromServer;
}

void AccountManager::handleAccountDeletionFromServerFailed(const QXmppStanza::Error &error)
{
    Q_EMIT accountDeletionFromClientAndServerFailed(error.text());

    if (m_deletionStates.testFlag(DeletionState::ClientDisconnectedBeforeDeletionFromServer)) {
        m_deletionStates = DeletionState::NotToBeDeleted;

        runOnThread(Kaidan::instance()->client(), []() {
            Kaidan::instance()->client()->logOut();
        });
    } else {
        m_deletionStates = DeletionState::NotToBeDeleted;
    }
}

bool AccountManager::handleConnected()
{
    // If the account could not be deleted because the client was disconnected, delete it now.
    if (m_deletionStates.testFlag(DeletionState::ToBeDeletedFromClient) && m_deletionStates.testFlag(DeletionState::ToBeDeletedFromServer)) {
        runOnThread(Kaidan::instance()->client(), []() {
            Kaidan::instance()->client()->registrationManager()->deleteAccount();
        });

        return true;
    } else if (m_deletionStates.testFlag(DeletionState::ToBeDeletedFromClient)) {
        deleteAccountFromClient();
        return true;
    }

    return false;
}

void AccountManager::handleDisconnected()
{
    // Delete the account from the client if the account was deleted from the server or the client
    // was connected and had to disconnect first.
    if (m_deletionStates.testFlag(DeletionState::DeletedFromServer)) {
        await(EncryptionController::instance()->resetLocally(), this, [this]() {
            removeAccount(account().jid);
        });
    } else if (m_deletionStates.testFlag(DeletionState::ToBeDeletedFromClient)) {
        removeAccount(account().jid);
    }

    m_deletionStates = DeletionState::NotToBeDeleted;
}

void AccountManager::removeAccount(const QString &accountJid)
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

#include "moc_AccountManager.cpp"
