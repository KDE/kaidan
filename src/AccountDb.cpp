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
#include "SqlUtils.h"

using namespace SqlUtils;

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

AccountDb::AccountDb(Database *db, QObject *parent)
    : DatabaseComponent(db, parent)
{
    Q_ASSERT(!AccountDb::s_instance);
    s_instance = this;
}

AccountDb::~AccountDb()
{
    s_instance = nullptr;
}

AccountDb *AccountDb::instance()
{
    return s_instance;
}

QFuture<void> AccountDb::addAccount(const QString &jid)
{
    return run([this, jid] {
        auto query = createQuery();
        execQuery(query,
                  QStringLiteral(R"(
				INSERT OR IGNORE INTO accounts (
					jid
				)
				VALUES (
					:jid
				)
			)"),
                  {
                      {u":jid", jid},
                  });
    });
}

QFuture<Account> AccountDb::account(const QString &jid)
{
    return run([this, jid]() {
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

        QList<Account> accounts;
        parseAccountsFromQuery(query, accounts);

        return accounts.first();
    });
}

QFuture<Account> AccountDb::lastAccount()
{
    // Get the last created account, which is the last we want to use.
    // When migrating, the old account is not deleted. Before we would store the jid to login in the settings file so we would retrieve it correctly
    // but now it's all asynchronous and tehre is no more settings. So the accoutn we need is the last one.
    // We may considere to just delete the previous account in db later, for now just don't change the behavior.
    return run([this]() {
        auto query = createQuery();
        execQuery(query, QStringLiteral(R"(SELECT * FROM accounts ORDER BY rowid DESC LIMIT 1)"));

        QList<Account> accounts;
        parseAccountsFromQuery(query, accounts);

        return accounts.isEmpty() ? Account() : accounts.constFirst();
    });
}

QFuture<void> AccountDb::updateAccount(const QString &jid, const std::function<void(Account &)> &updateAccount)
{
    return run([this, jid, updateAccount]() {
        // Load the account from the database.
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

        QList<Account> accounts;
        parseAccountsFromQuery(query, accounts);

        // Update the loaded account.
        if (!accounts.isEmpty()) {
            const auto &oldAccount = accounts.constFirst();
            Account newAccount = oldAccount;
            updateAccount(newAccount);

            // Replace the old account's values with the updated ones if the item has changed.
            if (oldAccount != newAccount) {
                Q_EMIT accountUpdated(newAccount);

                if (auto record = createUpdateRecord(oldAccount, newAccount); !record.isEmpty()) {
                    // Create an SQL record containing only the differences.
                    updateAccountByRecord(jid, record);
                }
            }
        }
    });
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
        auto query = createQuery();
        execQuery(query,
                  QStringLiteral(R"(
				DELETE FROM accounts WHERE jid = :jid
			)"),
                  {
                      {u":jid", jid},
                  });
    });
}

void AccountDb::parseAccountsFromQuery(QSqlQuery &query, QList<Account> &accounts)
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
    int idxOnline = rec.indexOf(QStringLiteral("online"));
    int idxResourcePrefix = rec.indexOf(QStringLiteral("resourcePrefix"));
    int idxPassword = rec.indexOf(QStringLiteral("password"));
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
        Account account;

        account.jid = query.value(idxJid).toString();
        account.latestMessageStanzaId = query.value(idxLatestMessageStanzaId).toString();
        account.latestMessageStanzaTimestamp = query.value(idxLatestMessageStanzaTimestamp).toDateTime();
        account.httpUploadLimit = query.value(idxHttpUploadLimit).toLongLong();

#define SET_IF(IDX, NAME, TYPE)                                                                                                                                \
    if (const auto NAME = query.value(IDX); !NAME.isNull())                                                                                                    \
    account.NAME = readValue<TYPE>(NAME)

        SET_IF(idxName, name, QString);
        SET_IF(idxContactNotificationRule, contactNotificationRule, Account::ContactNotificationRule);
        SET_IF(idxGroupChatNotificationRule, groupChatNotificationRule, Account::GroupChatNotificationRule);
        SET_IF(idxGeoLocationMapPreviewEnabled, geoLocationMapPreviewEnabled, bool);
        SET_IF(idxGeoLocationMapService, geoLocationMapService, Account::GeoLocationMapService);
        SET_IF(idxOnline, online, bool);
        SET_IF(idxResourcePrefix, resourcePrefix, QString);
        SET_IF(idxPassword, password, QString);
        // Password is stored as base64 unencrypted
        account.password = QString::fromUtf8(QByteArray::fromBase64(account.password.toUtf8()));
        SET_IF(idxCredentials, credentials, QXmppCredentials);
        SET_IF(idxHost, host, QString);
        SET_IF(idxPort, port, quint16);
        SET_IF(idxTlsErrorsIgnored, tlsErrorsIgnored, bool);
        SET_IF(idxTlsRequirement, tlsRequirement, QXmppConfiguration::StreamSecurityMode);
        SET_IF(idxPasswordVisibility, passwordVisibility, Kaidan::PasswordVisibility);
        SET_IF(idxUserAgentDeviceId, userAgentDeviceId, QUuid);
        SET_IF(idxEncryption, encryption, Encryption::Enum);
        SET_IF(idxAutomaticMediaDownloadsRule, automaticMediaDownloadsRule, Account::AutomaticMediaDownloadsRule);

#undef SET_IF

        accounts << std::move(account);
    }
}

QSqlRecord AccountDb::createUpdateRecord(const Account &oldAccount, const Account &newAccount)
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
    SET_IF_NEW(contactNotificationRule, Account::ContactNotificationRule);
    SET_IF_NEW(groupChatNotificationRule, Account::GroupChatNotificationRule);
    SET_IF_NEW(geoLocationMapPreviewEnabled, bool);
    SET_IF_NEW(geoLocationMapService, Account::GeoLocationMapService);

    SET_IF_NEW(online, bool);
    SET_IF_NEW(resourcePrefix, QString);
    SET_IF_NEW(password, QString);
    // Password is stored as base64 unencrypted
    if (auto field = rec.field(rec.count() - 1); field.name().compare(QStringLiteral("password"), Qt::CaseInsensitive) == 0) {
        if (!field.isNull()) {
            field.setValue(QString::fromUtf8(field.value().toString().toUtf8().toBase64()));
            rec.replace(rec.count() - 1, field);
        }
    }
    SET_IF_NEW(credentials, QXmppCredentials);
    SET_IF_NEW(host, QString);
    SET_IF_NEW(port, quint16);
    SET_IF_NEW(tlsErrorsIgnored, bool);
    SET_IF_NEW(tlsRequirement, QXmppConfiguration::StreamSecurityMode);
    SET_IF_NEW(passwordVisibility, Kaidan::PasswordVisibility);
    SET_IF_NEW(userAgentDeviceId, QUuid);
    SET_IF_NEW(encryption, Encryption::Enum);
    SET_IF_NEW(automaticMediaDownloadsRule, Account::AutomaticMediaDownloadsRule);

#undef SET_IF_NEW

    return rec;
}

void AccountDb::updateAccountByRecord(const QString &jid, const QSqlRecord &record)
{
    auto query = createQuery();
    auto &driver = sqlDriver();

    QMap<QString, QVariant> keyValuePairs = {{QStringLiteral("jid"), jid}};

    execQuery(query,
              driver.sqlStatement(QSqlDriver::UpdateStatement, QStringLiteral(DB_TABLE_ACCOUNTS), record, false)
                  + simpleWhereStatement(&driver, keyValuePairs));
}

#include "moc_AccountDb.cpp"
