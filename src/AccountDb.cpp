// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AccountDb.h"

// Qt
#include <QSqlDriver>
#include <QSqlField>
#include <QSqlQuery>
#include <QSqlRecord>
// Kaidan
#include "Account.h"
#include "Globals.h"
#include "KaidanCoreLog.h"
#include "Keychain.h"
#include "SqlUtils.h"

using namespace SqlUtils;
using namespace std::chrono;

static constexpr std::chrono::seconds KEYCHAIN_TIMEOUT = 15s;

template<typename T>
T readValue(const QVariant &value)
{
    if constexpr (std::is_same<T, QXmppCredentials>::value) {
        QXmlStreamReader r(value.toString());
        r.readNextStartElement();
        return QXmppCredentials::fromXml(r).value_or(QXmppCredentials());
    } else if constexpr (std::is_same<T, QUuid>::value) {
        return QUuid::fromString(value.toString());
    } else if constexpr (has_enum_type<T>::value || std::is_enum<T>::value) {
        return static_cast<T>(value.value<qint64>());
    }

    return value.value<T>();
}

template<typename T>
QVariant writeValue(const T &value)
{
    if constexpr (std::is_same<T, QXmppCredentials>::value) {
        QString xml;
        QXmlStreamWriter writer(&xml);
        value.toXml(writer);
        return xml;
    } else if constexpr (std::is_same<T, QUuid>::value) {
        return value.toString(QUuid::WithoutBraces);
    } else if constexpr (has_enum_type<T>::value || std::is_enum<T>::value) {
        return QVariant::fromValue(Enums::toIntegral(value));
    }

    return QVariant::fromValue(value);
}

AccountDb *AccountDb::s_instance = nullptr;

AccountDb *AccountDb::instance()
{
    return s_instance;
}

AccountDb::AccountDb(QObject *parent)
    : DatabaseComponent(parent)
{
    Q_ASSERT(!AccountDb::s_instance);
    s_instance = this;
}

AccountDb::~AccountDb()
{
    s_instance = nullptr;
}

QFuture<void> AccountDb::addAccount(const AccountSettings::Data &account)
{
    return run([this, account] {
        QKeychainFuture::waitForFinished(QKeychainFuture::writeKey(account.jid, account.password)
                                             .onFailed([account](const QKeychainFuture::Error &error) {
                                                 qCWarning(KAIDAN_CORE_LOG,
                                                           "Could not store account %ls since its password could not be stored: %s",
                                                           qUtf16Printable(account.jid),
                                                           error.what());
                                                 return error.error();
                                             })
                                             .then([this, account](const QKeychain::Error &error) {
                                                 if (error == QKeychain::Error::NoError) {
                                                     insert(QString::fromLatin1(DB_TABLE_ACCOUNTS),
                                                            {
                                                                {u"jid", account.jid},
                                                                {u"host", account.host.isEmpty() ? QVariant{} : account.host},
                                                                {u"name", account.name.isEmpty() ? QVariant{} : account.name},
                                                            });

                                                     Q_EMIT accountAdded(account);
                                                 }
                                             }),
                                         KEYCHAIN_TIMEOUT);
    });
}

QFuture<QList<AccountSettings::Data>> AccountDb::accounts()
{
    return run([this]() {
        auto query = createQuery();
        execQuery(query, QStringLiteral(R"(
				SELECT *
				FROM accounts
			)"));

        QList<AccountSettings::Data> accounts;
        parseAccountsFromQuery(query, accounts);

        return accounts;
    });
}

QFuture<void> AccountDb::updateAccount(const QString &jid, const std::function<void(AccountSettings::Data &)> &updateAccount)
{
    return run([this, jid, updateAccount]() {
        // Load the account settings from the database.
        auto query = createQuery();
        execQuery(query,
                  QStringLiteral(R"(
				SELECT *
				FROM accounts
				WHERE jid = :jid
			)"),
                  {
                      {u":jid", jid},
                  });

        QList<AccountSettings::Data> accounts;
        parseAccountsFromQuery(query, accounts);

        // Update the loaded account settings.
        if (!accounts.isEmpty()) {
            const auto &oldAccount = accounts.constFirst();
            AccountSettings::Data newAccount = oldAccount;
            updateAccount(newAccount);

            // Replace the old account settings with the updated ones if the item has changed.
            if (oldAccount != newAccount) {
                if (auto record = createUpdateRecord(oldAccount, newAccount); !record.isEmpty()) {
                    // Create an SQL record containing only the differences.
                    updateAccountByRecord(jid, record);
                }

                if (oldAccount.password != newAccount.password) {
                    QKeychainFuture::waitForFinished(
                        QKeychainFuture::writeKey(newAccount.jid, newAccount.password).onFailed([newAccount](const QKeychainFuture::Error &error) {
                            qCWarning(KAIDAN_CORE_LOG, "Could not update password of account %ls: %s", qUtf16Printable(newAccount.jid), error.what());
                            return error.error();
                        }),
                        KEYCHAIN_TIMEOUT);
                }
            }
        }
    });
}

QFuture<void> AccountDb::updateEnabled(const QString &jid, bool enabled)
{
    return updateField(jid, QStringLiteral("enabled"), enabled);
}

QFuture<void> AccountDb::updateHttpUploadLimit(const QString &jid, qint64 httpUploadLimit)
{
    return updateField(jid, QStringLiteral("httpUploadLimit"), httpUploadLimit);
}

QFuture<void> AccountDb::updatePasswordVisibility(const QString &jid, AccountSettings::PasswordVisibility passwordVisibility)
{
    return updateField(jid, QStringLiteral("passwordVisibility"), writeValue(passwordVisibility));
}

QFuture<void> AccountDb::updateEncryption(const QString &jid, Encryption::Enum encryption)
{
    return updateField(jid, QStringLiteral("encryption"), encryption);
}

QFuture<void> AccountDb::updateAutomaticMediaDownloadsRule(const QString &jid, AccountSettings::AutomaticMediaDownloadsRule automaticMediaDownloadsRule)
{
    return updateField(jid, QStringLiteral("automaticMediaDownloadsRule"), writeValue(automaticMediaDownloadsRule));
}

QFuture<void> AccountDb::updateContactNotificationRule(const QString &jid, AccountSettings::ContactNotificationRule contactNotificationRule)
{
    return updateField(jid, QStringLiteral("contactNotificationRule"), writeValue(contactNotificationRule));
}

QFuture<void> AccountDb::updateGroupChatNotificationRule(const QString &jid, AccountSettings::GroupChatNotificationRule groupChatNotificationRule)
{
    return updateField(jid, QStringLiteral("groupChatNotificationRule"), writeValue(groupChatNotificationRule));
}

QFuture<void> AccountDb::updateGeoLocationMapPreviewEnabled(const QString &jid, bool geoLocationMapPreviewEnabled)
{
    return updateField(jid, QStringLiteral("geoLocationMapPreviewEnabled"), geoLocationMapPreviewEnabled);
}

QFuture<void> AccountDb::updateGeoLocationMapService(const QString &jid, AccountSettings::GeoLocationMapService geoLocationMapService)
{
    return updateField(jid, QStringLiteral("geoLocationMapService"), writeValue(geoLocationMapService));
}

QFuture<QString> AccountDb::fetchLatestMessageStanzaId(const QString &jid)
{
    return run([this, jid]() {
        auto query = createQuery();
        execQuery(query,
                  QStringLiteral(R"(
				SELECT latestMessageStanzaId
				FROM accounts
				WHERE jid = :jid
			)"),
                  {
                      {u":jid", jid},
                  });

        QString stanzaId;

        if (query.first()) {
            stanzaId = query.value(0).toString();
        }

        return stanzaId;
    });
}

QFuture<void> AccountDb::removeAccount(const QString &jid)
{
    return run([this, jid]() {
        QKeychainFuture::waitForFinished(
            QKeychainFuture::deleteKey(jid)
                .onFailed([jid](const QKeychainFuture::Error &error) {
                    qCWarning(KAIDAN_CORE_LOG, "Could not remove account %ls since its password could not be removed: %s", qUtf16Printable(jid), error.what());
                    return error.error();
                })
                .then([this, jid](const QKeychain::Error &error) {
                    if (error == QKeychain::Error::NoError) {
                        auto query = createQuery();
                        execQuery(query,
                                  QStringLiteral(R"(DELETE FROM accounts WHERE jid = :jid)"),
                                  {
                                      {u":jid", jid},
                                  });

                        Q_EMIT accountRemoved(jid);
                    }
                }),
            KEYCHAIN_TIMEOUT);
    });
}

void AccountDb::parseAccountsFromQuery(QSqlQuery &query, QList<AccountSettings::Data> &accounts)
{
    QSqlRecord rec = query.record();

    int idxJid = rec.indexOf(QStringLiteral("jid"));
    int idxName = rec.indexOf(QStringLiteral("name"));
    int idxLatestMessageStanzaId = rec.indexOf(QStringLiteral("latestMessageStanzaId"));
    int idxLatestMessageStanzaTimestamp = rec.indexOf(QStringLiteral("latestMessageStanzaTimestamp"));
    int idxHttpUploadLimit = rec.indexOf(QStringLiteral("httpUploadLimit"));
    int idxContactNotificationRule = rec.indexOf(QStringLiteral("contactNotificationRule"));
    int idxGroupChatNotificationRule = rec.indexOf(QStringLiteral("groupChatNotificationRule"));
    int idxGeoLocationMapPreviewEnabled = rec.indexOf(QStringLiteral("geoLocationMapPreviewEnabled"));
    int idxGeoLocationMapService = rec.indexOf(QStringLiteral("geoLocationMapService"));
    int idxEnabled = rec.indexOf(QStringLiteral("enabled"));
    int idxJidResourcePrefix = rec.indexOf(QStringLiteral("resourcePrefix"));
    int idxCredentials = rec.indexOf(QStringLiteral("credentials"));
    int idxHost = rec.indexOf(QStringLiteral("host"));
    int idxPort = rec.indexOf(QStringLiteral("port"));
    int idxTlsErrorsIgnored = rec.indexOf(QStringLiteral("tlsErrorsIgnored"));
    int idxTlsRequirement = rec.indexOf(QStringLiteral("tlsRequirement"));
    int idxPasswordVisibility = rec.indexOf(QStringLiteral("passwordVisibility"));
    int idxUserAgentDeviceId = rec.indexOf(QStringLiteral("userAgentDeviceId"));
    int idxEncryption = rec.indexOf(QStringLiteral("encryption"));
    int idxAutomaticMediaDownloadsRule = rec.indexOf(QStringLiteral("automaticMediaDownloadsRule"));

    reserve(accounts, query);
    while (query.next()) {
        AccountSettings::Data account;

        account.initialized = true;
        account.initialMessagesRetrieved = true;
        account.jid = query.value(idxJid).toString();
        account.latestMessageStanzaId = query.value(idxLatestMessageStanzaId).toString();
        account.latestMessageStanzaTimestamp = query.value(idxLatestMessageStanzaTimestamp).toDateTime();
        account.httpUploadLimit = query.value(idxHttpUploadLimit).toLongLong();

#define SET_IF(IDX, NAME, TYPE)                                                                                                                                \
    if (const auto NAME = query.value(IDX); !NAME.isNull())                                                                                                    \
    account.NAME = readValue<TYPE>(NAME)

        SET_IF(idxName, name, QString);
        SET_IF(idxContactNotificationRule, contactNotificationRule, AccountSettings::ContactNotificationRule);
        SET_IF(idxGroupChatNotificationRule, groupChatNotificationRule, AccountSettings::GroupChatNotificationRule);
        SET_IF(idxGeoLocationMapPreviewEnabled, geoLocationMapPreviewEnabled, bool);
        SET_IF(idxGeoLocationMapService, geoLocationMapService, AccountSettings::GeoLocationMapService);
        SET_IF(idxEnabled, enabled, bool);
        SET_IF(idxJidResourcePrefix, jidResourcePrefix, QString);
        SET_IF(idxCredentials, credentials, QXmppCredentials);
        SET_IF(idxHost, host, QString);
        SET_IF(idxPort, port, quint16);
        SET_IF(idxTlsErrorsIgnored, tlsErrorsIgnored, bool);
        SET_IF(idxTlsRequirement, tlsRequirement, QXmppConfiguration::StreamSecurityMode);
        SET_IF(idxPasswordVisibility, passwordVisibility, AccountSettings::PasswordVisibility);
        SET_IF(idxUserAgentDeviceId, userAgentDeviceId, QUuid);
        SET_IF(idxEncryption, encryption, Encryption::Enum);
        SET_IF(idxAutomaticMediaDownloadsRule, automaticMediaDownloadsRule, AccountSettings::AutomaticMediaDownloadsRule);

        QKeychainFuture::waitForFinished(
            QKeychainFuture::readKey<QString>(account.jid)
                .onFailed([account](const QKeychainFuture::Error &error) {
                    qCWarning(KAIDAN_CORE_LOG, "Could not retrieve password for account %ls: %s", qUtf16Printable(account.jid), error.what());
                    return QString();
                })
                .then([&account](const QKeychainFuture::ReadResult<QString> &result) {
                    if (auto password = std::get_if<QString>(&result)) {
                        account.password = *password;
                    }
                }),
            KEYCHAIN_TIMEOUT);

#undef SET_IF

        accounts << std::move(account);
    }
}

QSqlRecord AccountDb::createUpdateRecord(const AccountSettings::Data &oldAccount, const AccountSettings::Data &newAccount)
{
    QSqlRecord rec;

#define SET_IF_NEW(NAME, TYPE)                                                                                                                                 \
    if (oldAccount.NAME != newAccount.NAME)                                                                                                                    \
    rec.append(createSqlField(QStringLiteral(QT_STRINGIFY(NAME)), writeValue<TYPE>(newAccount.NAME)))

    SET_IF_NEW(jid, QString);
    SET_IF_NEW(name, QString);
    SET_IF_NEW(latestMessageStanzaId, QString);
    SET_IF_NEW(latestMessageStanzaTimestamp, QDateTime);
    SET_IF_NEW(httpUploadLimit, qint64);
    SET_IF_NEW(contactNotificationRule, AccountSettings::ContactNotificationRule);
    SET_IF_NEW(groupChatNotificationRule, AccountSettings::GroupChatNotificationRule);
    SET_IF_NEW(geoLocationMapPreviewEnabled, bool);
    SET_IF_NEW(geoLocationMapService, AccountSettings::GeoLocationMapService);
    SET_IF_NEW(enabled, bool);
    SET_IF_NEW(jidResourcePrefix, QString);
    SET_IF_NEW(credentials, QXmppCredentials);
    SET_IF_NEW(host, QString);
    SET_IF_NEW(port, quint16);
    SET_IF_NEW(tlsErrorsIgnored, bool);
    SET_IF_NEW(tlsRequirement, QXmppConfiguration::StreamSecurityMode);
    SET_IF_NEW(passwordVisibility, AccountSettings::PasswordVisibility);
    SET_IF_NEW(userAgentDeviceId, QUuid);
    SET_IF_NEW(encryption, Encryption::Enum);
    SET_IF_NEW(automaticMediaDownloadsRule, AccountSettings::AutomaticMediaDownloadsRule);

#undef SET_IF_NEW

    return rec;
}

void AccountDb::updateAccountByRecord(const QString &jid, const QSqlRecord &record)
{
    auto query = createQuery();
    auto &driver = sqlDriver();

    execQuery(query,
              driver.sqlStatement(QSqlDriver::UpdateStatement, QStringLiteral(DB_TABLE_ACCOUNTS), record, false)
                  + simpleWhereStatement(&driver, QStringLiteral("jid"), jid));
}

QFuture<void> AccountDb::updateField(const QString &jid, const QString &column, const QVariant &value)
{
    return run([this, jid, column, value]() {
        auto query = createQuery();
        auto &driver = sqlDriver();

        QSqlRecord record;
        record.append(createSqlField(column, value));

        execQuery(query,
                  driver.sqlStatement(QSqlDriver::UpdateStatement, QStringLiteral(DB_TABLE_ACCOUNTS), record, false)
                      + simpleWhereStatement(&driver, QStringLiteral("jid"), jid));
    });
}

#include "moc_AccountDb.cpp"
