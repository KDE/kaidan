// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Xavier <xavi@delape.net>
// SPDX-FileCopyrightText: 2020 Yury Gubich <blue@macaw.me>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
// SPDX-FileCopyrightText: 2023 Sergey Smirnykh <sergey.smirnykh@siborgium.xyz>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Database.h"

// Qt
#include <QDir>
#include <QMutex>
#include <QRandomGenerator>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlField>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QStringBuilder>
#include <QThreadPool>
#include <QThreadStorage>
#include <QtConcurrent/QtConcurrentRun>
// Kaidan
#include "Account.h"
#include "AccountDb.h"
#include "Globals.h"
#include "KaidanCoreLog.h"
#include "MainController.h"
#include "Settings.h"
#include "SqlUtils.h"

using namespace SqlUtils;

#define DATABASE_CONVERT_TO_VERSION(n)                                                                                                                         \
    if (d->version < n) {                                                                                                                                      \
        convertDatabaseToV##n();                                                                                                                               \
    }

// Both need to be updated on version bump:
#define DATABASE_LATEST_VERSION 50
#define DATABASE_CONVERT_TO_LATEST_VERSION() DATABASE_CONVERT_TO_VERSION(50)

#define SQL_BOOL "BOOL"
#define SQL_BOOL_NOT_NULL "BOOL NOT NULL"
#define SQL_INTEGER "INTEGER"
#define SQL_INTEGER_NOT_NULL "INTEGER NOT NULL"
#define SQL_TEXT "TEXT"
#define SQL_TEXT_NOT_NULL "TEXT NOT NULL"
#define SQL_BLOB "BLOB"
#define SQL_BLOB_NOT_NULL "BLOB NOT NULL"

#define SQL_CREATE_TABLE(tableName, contents) QStringLiteral("CREATE TABLE '" tableName "' (" contents ")")

#define SQL_LAST_ATTRIBUTE(name, dataType) "'" QT_STRINGIFY(name) "' " dataType

#define SQL_ATTRIBUTE(name, dataType) SQL_LAST_ATTRIBUTE(name, dataType) ","

class DbConnection;

static QThreadStorage<DbConnection *> dbConnections;

class DbConnection
{
    Q_DISABLE_COPY(DbConnection)
public:
    DbConnection()
        : m_name(QString::number(QRandomGenerator::global()->generate(), 36))
    {
        auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_name);
        if (!database.isValid()) {
            qFatal("Cannot add database: %s", qPrintable(database.lastError().text()));
        }

        const auto databaseFilePath = []() {
            // Check if there is a writable location for app data.
            const auto appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
            if (appDataPath.isEmpty()) {
                qFatal("Failed to find writable location for database file.");
            }

            const auto appDataDir = QDir(appDataPath);
            if (!appDataDir.mkpath(QLatin1String("."))) {
                qFatal("Failed to create writable directory at %s", qPrintable(appDataDir.absolutePath()));
            }

            // Create the absolute database file path while ensuring that there is a writable
            // location on all systems.
            const auto databaseFilePath = appDataDir.absoluteFilePath(databaseFilename());

            // Rename old database files to the current name.
            // There should only be one old file at once.
            // Thus, only the first entry of "oldDbFilenames" is renamed.
            if (const auto oldDbFilenames = appDataDir.entryList(oldDatabaseFilenames(), QDir::Files); !oldDbFilenames.isEmpty()) {
                QFile::rename(appDataDir.absoluteFilePath(oldDbFilenames.first()), databaseFilePath);
            }

            return databaseFilePath;
        }();

        // open() creates the database file if it doesn't exist.
        database.setDatabaseName(databaseFilePath);
        if (!database.open()) {
            qFatal("Cannot open database: %s", qPrintable(database.lastError().text()));
        }
    }

    ~DbConnection()
    {
        QSqlDatabase::removeDatabase(m_name);
    }

    QSqlDatabase database()
    {
        return QSqlDatabase::database(m_name);
    }

private:
    QString m_name;
};

enum DatabaseVersion {
    DbNotLoaded = -1,
    DbNotCreated = 0, // no tables
    DbOldVersion = 1, // Kaidan v0.3 or before
};

struct DatabasePrivate {
    QThread dbThread;
    QObject *dbWorker = new QObject();
    QMutex tableCreationMutex;
    int version = DbNotLoaded;
    int transactions = 0;
    bool tablesCreated = false;
};

Database *Database::s_instance = nullptr;

Database *Database::instance()
{
    return s_instance;
}

Database::Database(QObject *parent)
    : QObject(parent)
    , d(new DatabasePrivate)
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    // start thread and set thread name
    d->dbThread.setObjectName(QStringLiteral("DB (SQLite)"));
    d->dbThread.start();
    // worker
    d->dbWorker->moveToThread(&d->dbThread);
    connect(&d->dbThread, &QThread::finished, d->dbWorker, &QObject::deleteLater);
}

Database::~Database()
{
    // wait for finished
    d->dbThread.quit();
    d->dbThread.wait();

    s_instance = nullptr;
}

void Database::createTables()
{
    QMutexLocker locker(&d->tableCreationMutex);
    if (d->tablesCreated) {
        return;
    }

    loadDatabaseInfo();

    if (needToConvert())
        convertDatabase();

    d->tablesCreated = true;
}

void Database::createV3Database()
{
    QSqlQuery query(currentDatabase());

    execQuery(query, SQL_CREATE_TABLE(DB_TABLE_INFO, SQL_LAST_ATTRIBUTE(version, SQL_INTEGER_NOT_NULL)));

    execQuery(query,
              SQL_CREATE_TABLE("Roster",
                               SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(name, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(lastExchanged, SQL_TEXT_NOT_NULL)
                                   SQL_ATTRIBUTE(unreadMessages, SQL_INTEGER) SQL_ATTRIBUTE(lastMessage, SQL_TEXT) SQL_ATTRIBUTE(lastOnline, SQL_TEXT)
                                       SQL_ATTRIBUTE(activity, SQL_TEXT) SQL_ATTRIBUTE(status, SQL_TEXT) SQL_ATTRIBUTE(mood, SQL_TEXT)
                                           SQL_LAST_ATTRIBUTE(avatarHash, SQL_TEXT)));

    execQuery(query,
              SQL_CREATE_TABLE("Messages",
                               SQL_ATTRIBUTE(author, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(author_resource, SQL_TEXT) SQL_ATTRIBUTE(recipient, SQL_TEXT_NOT_NULL)
                                   SQL_ATTRIBUTE(recipient_resource, SQL_TEXT) SQL_ATTRIBUTE(timestamp, SQL_TEXT_NOT_NULL)
                                       SQL_ATTRIBUTE(message, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(id, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(isSent, SQL_BOOL)
                                           SQL_ATTRIBUTE(isDelivered, SQL_BOOL) "FOREIGN KEY('author') REFERENCES Roster ('jid'),"
                                                                                "FOREIGN KEY('recipient') REFERENCES Roster ('jid')"));

    d->version = 3;
}

void Database::startTransaction()
{
    QMetaObject::invokeMethod(d->dbWorker, [this] {
        transaction();
    });
}

void Database::commitTransaction()
{
    QMetaObject::invokeMethod(d->dbWorker, [this] {
        commit();
    });
}

void Database::transaction()
{
    auto &transactions = activeTransactions();
    if (!transactions) {
        auto db = currentDatabase();
        // currently no transactions running
        if (!db.transaction()) {
            qCWarning(KAIDAN_CORE_LOG) << "Could not begin transaction on database:" << db.lastError().text();
        }
    }
    // increase counter
    transactions++;
}

void Database::commit()
{
    auto &transactions = activeTransactions();

    // reduce counter
    transactions--;
    if (transactions < 0) {
        transactions = 0;
        qCWarning(KAIDAN_CORE_LOG) << "commit() called but no transaction to commit";
    }

    if (!transactions) {
        // no transaction requested anymore
        auto db = currentDatabase();
        if (!db.commit()) {
            qCWarning(KAIDAN_CORE_LOG) << "Could not commit transaction on database:" << db.lastError().text();
        }
    }
}

QObject *Database::dbWorker() const
{
    return d->dbWorker;
}

QSqlDatabase Database::currentDatabase()
{
    if (!dbConnections.hasLocalData()) {
        dbConnections.setLocalData(new DbConnection());
    }
    return dbConnections.localData()->database();
}

QSqlQuery Database::createQuery()
{
    if (!d->tablesCreated) {
        createTables();
    }
    QSqlQuery query(currentDatabase());
    query.setForwardOnly(true);
    return query;
}

void Database::loadDatabaseInfo()
{
    auto db = currentDatabase();
    const auto tables = db.tables();
    if (!tables.contains(QStringLiteral(DB_TABLE_INFO))) {
        if (tables.contains(QStringLiteral("Messages")) && tables.contains(QStringLiteral("Roster"))) {
            // old Kaidan v0.1/v0.2 table
            d->version = DbOldVersion;
        } else {
            d->version = DbNotCreated;
        }
        // we've got all we want; do not query for a db version
        return;
    }

    QSqlQuery query(db);
    execQuery(query, QStringLiteral("SELECT version FROM " DB_TABLE_INFO));

    QSqlRecord record = query.record();
    int versionCol = record.indexOf(QStringLiteral("version"));

    while (query.next()) {
        d->version = query.value(versionCol).toInt();
    }
}

void Database::saveDatabaseInfo()
{
    if (d->version <= DbOldVersion || d->version > DATABASE_LATEST_VERSION) {
        qFatal("[Database] Fatal error: Attempted to save invalid db version number.");
    }

    QSqlRecord updateRecord;
    updateRecord.append(createSqlField(QStringLiteral("version"), d->version));

    auto db = currentDatabase();
    QSqlQuery query(db);
    execQuery(query, db.driver()->sqlStatement(QSqlDriver::UpdateStatement, QStringLiteral(DB_TABLE_INFO), updateRecord, false));
}

int &Database::activeTransactions()
{
    thread_local static int activeTransactions = 0;
    return activeTransactions;
}

bool Database::needToConvert()
{
    return d->version < DATABASE_LATEST_VERSION;
}

void Database::convertDatabase()
{
    transaction();

    if (d->version == DbNotCreated) {
        qCDebug(KAIDAN_CORE_LOG) << "Creating new database with latest version" << DATABASE_LATEST_VERSION;
        createNewDatabase();
    } else {
        qCDebug(KAIDAN_CORE_LOG) << "Converting database from version" << d->version << "to latest version" << DATABASE_LATEST_VERSION;
        DATABASE_CONVERT_TO_LATEST_VERSION();
    }

    saveDatabaseInfo();
    commit();
}

void Database::createNewDatabase()
{
    auto db = currentDatabase();
    QSqlQuery query(db);

    // DBINFO
    execQuery(query, SQL_CREATE_TABLE(DB_TABLE_INFO, SQL_LAST_ATTRIBUTE(version, SQL_INTEGER_NOT_NULL)));

    QSqlRecord insertRecord;
    insertRecord.append(createSqlField(QStringLiteral("version"), DATABASE_LATEST_VERSION));
    execQuery(query, db.driver()->sqlStatement(QSqlDriver::InsertStatement, QStringLiteral(DB_TABLE_INFO), insertRecord, false));

    // accounts
    execQuery(query,
              SQL_CREATE_TABLE(DB_TABLE_ACCOUNTS,
                               SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(name, SQL_TEXT) SQL_ATTRIBUTE(latestMessageStanzaId, SQL_TEXT)
                                   SQL_ATTRIBUTE(latestMessageStanzaTimestamp, SQL_TEXT) SQL_ATTRIBUTE(httpUploadLimit, SQL_INTEGER)
                                       SQL_ATTRIBUTE(contactNotificationRule, SQL_INTEGER) SQL_ATTRIBUTE(groupChatNotificationRule, SQL_INTEGER)
                                           SQL_ATTRIBUTE(geoLocationMapPreviewEnabled, SQL_BOOL) SQL_ATTRIBUTE(geoLocationMapService, SQL_INTEGER)
                                               SQL_ATTRIBUTE(enabled, SQL_BOOL) SQL_ATTRIBUTE(resourcePrefix, SQL_TEXT) SQL_ATTRIBUTE(password, SQL_TEXT)
                                                   SQL_ATTRIBUTE(credentials, SQL_TEXT) SQL_ATTRIBUTE(host, SQL_TEXT) SQL_ATTRIBUTE(port, SQL_BOOL)
                                                       SQL_ATTRIBUTE(tlsErrorsIgnored, SQL_INTEGER) SQL_ATTRIBUTE(tlsRequirement, SQL_INTEGER)
                                                           SQL_ATTRIBUTE(passwordVisibility, SQL_INTEGER) SQL_ATTRIBUTE(userAgentDeviceId, SQL_TEXT)
                                                               SQL_ATTRIBUTE(encryption, SQL_INTEGER)
                                                                   SQL_ATTRIBUTE(automaticMediaDownloadsRule, SQL_INTEGER) "PRIMARY KEY(jid)"));

    // roster
    execQuery(query,
              SQL_CREATE_TABLE(DB_TABLE_ROSTER,
                               SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(name, SQL_TEXT)
                                   SQL_ATTRIBUTE(subscription, SQL_INTEGER) SQL_ATTRIBUTE(groupChatParticipantId, SQL_TEXT)
                                       SQL_ATTRIBUTE(groupChatName, SQL_TEXT) SQL_ATTRIBUTE(groupChatDescription, SQL_TEXT)
                                           SQL_ATTRIBUTE(groupChatFlags, SQL_INTEGER) SQL_ATTRIBUTE(encryption, SQL_INTEGER)
                                               SQL_ATTRIBUTE(unreadMessages, SQL_INTEGER) SQL_ATTRIBUTE(lastReadOwnMessageId, SQL_TEXT)
                                                   SQL_ATTRIBUTE(lastReadContactMessageId, SQL_TEXT) SQL_ATTRIBUTE(latestGroupChatMessageStanzaId, SQL_TEXT)
                                                       SQL_ATTRIBUTE(latestGroupChatMessageStanzaTimestamp, SQL_TEXT) SQL_ATTRIBUTE(readMarkerPending, SQL_BOOL)
                                                           SQL_ATTRIBUTE(pinningPosition, SQL_INTEGER_NOT_NULL) SQL_ATTRIBUTE(chatStateSendingEnabled, SQL_BOOL)
                                                               SQL_ATTRIBUTE(readMarkerSendingEnabled, SQL_BOOL) SQL_ATTRIBUTE(notificationRule, SQL_INTEGER)
                                                                   SQL_ATTRIBUTE(automaticMediaDownloadsRule, SQL_INTEGER) "PRIMARY KEY(accountJid, jid)"));
    execQuery(query,
              SQL_CREATE_TABLE(DB_TABLE_ROSTER_GROUPS,
                               SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(chatJid, SQL_TEXT_NOT_NULL)
                                   SQL_ATTRIBUTE(name, SQL_TEXT_NOT_NULL) "PRIMARY KEY(accountJid, chatJid, name),"
                                                                          "FOREIGN KEY(accountJid) REFERENCES " DB_TABLE_ROSTER " (accountJid),"
                                                                          "FOREIGN KEY(chatJid) REFERENCES " DB_TABLE_ROSTER " (jid)"));

    // messages
    execQuery(
        query,
        SQL_CREATE_TABLE(
            DB_TABLE_MESSAGES,
            SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(chatJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(isOwn, SQL_BOOL_NOT_NULL)
                SQL_ATTRIBUTE(groupChatSenderId, SQL_TEXT) SQL_ATTRIBUTE(id, SQL_TEXT) SQL_ATTRIBUTE(originId, SQL_TEXT) SQL_ATTRIBUTE(stanzaId, SQL_TEXT)
                    SQL_ATTRIBUTE(replaceId, SQL_TEXT) SQL_ATTRIBUTE(replyTo, SQL_TEXT) SQL_ATTRIBUTE(replyId, SQL_TEXT) SQL_ATTRIBUTE(replyQuote, SQL_TEXT)
                        SQL_ATTRIBUTE(timestamp, SQL_TEXT) SQL_ATTRIBUTE(body, SQL_TEXT) SQL_ATTRIBUTE(encryption, SQL_INTEGER)
                            SQL_ATTRIBUTE(senderKey, SQL_BLOB) SQL_ATTRIBUTE(deliveryState, SQL_INTEGER) SQL_ATTRIBUTE(isSpoiler, SQL_BOOL)
                                SQL_ATTRIBUTE(spoilerHint, SQL_TEXT) SQL_ATTRIBUTE(fileGroupId, SQL_INTEGER) SQL_ATTRIBUTE(groupChatInviterJid, SQL_TEXT)
                                    SQL_ATTRIBUTE(groupChatInviteeJid, SQL_TEXT) SQL_ATTRIBUTE(groupChatInvitationJid, SQL_TEXT)
                                        SQL_ATTRIBUTE(groupChatToken, SQL_TEXT) SQL_ATTRIBUTE(errorText, SQL_TEXT)
                                            SQL_ATTRIBUTE(removed, SQL_BOOL_NOT_NULL) "FOREIGN KEY(accountJid, chatJid) REFERENCES roster (accountJid, jid)"));
    execQuery(query,
              SQL_CREATE_TABLE(DB_TABLE_MESSAGE_REACTIONS,
                               SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(chatJid, SQL_TEXT_NOT_NULL)
                                   SQL_ATTRIBUTE(messageSenderId, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(messageId, SQL_TEXT_NOT_NULL)
                                       SQL_ATTRIBUTE(senderId, SQL_TEXT) SQL_ATTRIBUTE(emoji, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(timestamp, SQL_INTEGER)
                                           SQL_ATTRIBUTE(deliveryState, SQL_INTEGER) "PRIMARY KEY(accountJid, chatJid, messageId, senderId, emoji)"));

    // group chats
    execQuery(query,
              SQL_CREATE_TABLE(DB_TABLE_GROUP_CHAT_USERS,
                               SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(chatJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(id, SQL_TEXT_NOT_NULL)
                                   SQL_ATTRIBUTE(jid, SQL_TEXT) SQL_ATTRIBUTE(name, SQL_TEXT)
                                       SQL_ATTRIBUTE(status, SQL_INTEGER) "PRIMARY KEY(accountJid, chatJid, id, jid),"
                                                                          "FOREIGN KEY(accountJid, chatJid) REFERENCES " DB_TABLE_ROSTER " (accountJid, jid)"));

    // file sharing
    execQuery(query,
              SQL_CREATE_TABLE(DB_TABLE_FILES,
                               SQL_ATTRIBUTE(id, SQL_INTEGER_NOT_NULL) SQL_ATTRIBUTE(fileGroupId, SQL_INTEGER_NOT_NULL) SQL_ATTRIBUTE(name, SQL_TEXT)
                                   SQL_ATTRIBUTE(description, SQL_TEXT) SQL_ATTRIBUTE(mimeType, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(size, SQL_INTEGER)
                                       SQL_ATTRIBUTE(lastModified, SQL_INTEGER_NOT_NULL) SQL_ATTRIBUTE(disposition, SQL_INTEGER_NOT_NULL)
                                           SQL_ATTRIBUTE(thumbnail, SQL_BLOB) SQL_ATTRIBUTE(localFilePath, SQL_TEXT)
                                               SQL_ATTRIBUTE(externalId, SQL_TEXT) "PRIMARY KEY(id)"));
    execQuery(query,
              SQL_CREATE_TABLE(DB_TABLE_FILE_HASHES,
                               SQL_ATTRIBUTE(dataId, SQL_INTEGER_NOT_NULL) SQL_ATTRIBUTE(hashType, SQL_INTEGER_NOT_NULL)
                                   SQL_ATTRIBUTE(hashValue, SQL_BLOB_NOT_NULL) "PRIMARY KEY(dataId, hashType)"));
    execQuery(
        query,
        SQL_CREATE_TABLE(DB_TABLE_FILE_HTTP_SOURCES, SQL_ATTRIBUTE(fileId, SQL_INTEGER_NOT_NULL) SQL_ATTRIBUTE(url, SQL_BLOB_NOT_NULL) "PRIMARY KEY(fileId)"));
    execQuery(query,
              SQL_CREATE_TABLE(DB_TABLE_FILE_ENCRYPTED_SOURCES,
                               SQL_ATTRIBUTE(fileId, SQL_INTEGER_NOT_NULL) SQL_ATTRIBUTE(url, SQL_BLOB_NOT_NULL) SQL_ATTRIBUTE(cipher, SQL_INTEGER_NOT_NULL)
                                   SQL_ATTRIBUTE(key, SQL_BLOB_NOT_NULL) SQL_ATTRIBUTE(iv, SQL_BLOB_NOT_NULL)
                                       SQL_ATTRIBUTE(encryptedDataId, SQL_INTEGER) "PRIMARY KEY(fileId)"));

    // trust management
    execQuery(query,
              SQL_CREATE_TABLE(DB_TABLE_TRUST_SECURITY_POLICIES,
                               SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(encryption, SQL_TEXT_NOT_NULL)
                                   SQL_ATTRIBUTE(securityPolicy, SQL_INTEGER_NOT_NULL) "PRIMARY KEY(account, encryption)"));
    execQuery(query,
              SQL_CREATE_TABLE(DB_TABLE_TRUST_OWN_KEYS,
                               SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(encryption, SQL_TEXT_NOT_NULL)
                                   SQL_ATTRIBUTE(keyId, SQL_BLOB_NOT_NULL) "PRIMARY KEY(account, encryption)"));
    execQuery(query,
              SQL_CREATE_TABLE(DB_TABLE_TRUST_KEYS,
                               SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(encryption, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(ownerJid, SQL_TEXT_NOT_NULL)
                                   SQL_ATTRIBUTE(keyId, SQL_BLOB_NOT_NULL)
                                       SQL_ATTRIBUTE(trustLevel, SQL_INTEGER_NOT_NULL) "PRIMARY KEY(account, encryption, keyId, ownerJid)"));
    execQuery(query,
              SQL_CREATE_TABLE(DB_TABLE_TRUST_KEYS_UNPROCESSED,
                               SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(encryption, SQL_TEXT_NOT_NULL)
                                   SQL_ATTRIBUTE(senderKeyId, SQL_BLOB_NOT_NULL) SQL_ATTRIBUTE(ownerJid, SQL_TEXT_NOT_NULL)
                                       SQL_ATTRIBUTE(keyId, SQL_BLOB_NOT_NULL)
                                           SQL_ATTRIBUTE(trust, SQL_BOOL_NOT_NULL) "PRIMARY KEY(account, encryption, keyId, ownerJid)"));

    // OMEMO data
    execQuery(query,
              SQL_CREATE_TABLE(DB_TABLE_OMEMO_OWN_DEVICES,
                               SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(id, SQL_INTEGER_NOT_NULL) SQL_ATTRIBUTE(label, SQL_TEXT)
                                   SQL_ATTRIBUTE(privateKey, SQL_BLOB) SQL_ATTRIBUTE(publicKey, SQL_BLOB)
                                       SQL_ATTRIBUTE(latestSignedPreKeyId, SQL_INTEGER_NOT_NULL)
                                           SQL_ATTRIBUTE(latestPreKeyId, SQL_INTEGER_NOT_NULL) "PRIMARY KEY(account)"));
    execQuery(query,
              SQL_CREATE_TABLE(DB_TABLE_OMEMO_DEVICES,
                               SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(userJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(id, SQL_INTEGER_NOT_NULL)
                                   SQL_ATTRIBUTE(label, SQL_TEXT) SQL_ATTRIBUTE(keyId, SQL_BLOB) SQL_ATTRIBUTE(session, SQL_BLOB)
                                       SQL_ATTRIBUTE(unrespondedStanzasSent, SQL_INTEGER " DEFAULT 0")
                                           SQL_ATTRIBUTE(unrespondedStanzasReceived, SQL_INTEGER " DEFAULT 0")
                                               SQL_ATTRIBUTE(removalTimestamp, SQL_INTEGER) "PRIMARY KEY(account, userJid, id)"));
    execQuery(query,
              SQL_CREATE_TABLE(DB_TABLE_OMEMO_PRE_KEY_PAIRS,
                               SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(id, SQL_BLOB_NOT_NULL)
                                   SQL_ATTRIBUTE(data, SQL_BLOB_NOT_NULL) "PRIMARY KEY(account, id)"));
    execQuery(query,
              SQL_CREATE_TABLE(DB_TABLE_OMEMO_SIGNED_PRE_KEY_PAIRS,
                               SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(id, SQL_BLOB_NOT_NULL) SQL_ATTRIBUTE(data, SQL_BLOB_NOT_NULL)
                                   SQL_ATTRIBUTE(creationTimestamp, SQL_INTEGER) "PRIMARY KEY(account, id)"));

    // blocked
    execQuery(
        query,
        SQL_CREATE_TABLE(DB_TABLE_BLOCKED, SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL) "PRIMARY KEY(accountJid, jid)"));

    execQuery(query, QStringLiteral("CREATE VIEW " DB_VIEW_CHAT_MESSAGES " AS SELECT * FROM " DB_TABLE_MESSAGES " WHERE deliveryState != 4 AND removed != 1"));
    execQuery(query, QStringLiteral("CREATE VIEW " DB_VIEW_DRAFT_MESSAGES " AS SELECT * FROM " DB_TABLE_MESSAGES " WHERE deliveryState = 4"));

    d->version = DATABASE_LATEST_VERSION;
}

void Database::convertDatabaseToV2()
{
    // create a new dbinfo table
    QSqlQuery query(currentDatabase());
    execQuery(query, SQL_CREATE_TABLE(DB_TABLE_INFO, SQL_LAST_ATTRIBUTE(version, SQL_INTEGER_NOT_NULL)));
    execQuery(query, QStringLiteral("INSERT INTO dbinfo VALUES (:version)"), {{u":version", 2}});
    d->version = 2;
}

void Database::convertDatabaseToV3()
{
    DATABASE_CONVERT_TO_VERSION(2);
    QSqlQuery query(currentDatabase());
    execQuery(query, QStringLiteral("ALTER TABLE Roster ADD avatarHash " SQL_TEXT));
    d->version = 3;
}

void Database::convertDatabaseToV4()
{
    DATABASE_CONVERT_TO_VERSION(3);
    QSqlQuery query(currentDatabase());
    // SQLite doesn't support the ALTER TABLE drop columns feature, so we have to use a workaround.
    // we copy all rows into a back-up table (but without `avatarHash`), and then delete the old table
    // and copy everything to the normal table again
    execQuery(query,
              QStringLiteral("CREATE TEMPORARY TABLE roster_backup(jid, name, lastExchanged,"
                             "unreadMessages, lastMessage, lastOnline, activity, status, mood)"));
    execQuery(query,
              QStringLiteral("INSERT INTO roster_backup SELECT jid, name, lastExchanged, unreadMessages,"
                             "lastMessage, lastOnline, activity, status, mood FROM " DB_TABLE_ROSTER));
    execQuery(query, QStringLiteral("DROP TABLE Roster"));
    execQuery(query,
              QStringLiteral("CREATE TABLE Roster (jid " SQL_TEXT_NOT_NULL ", name " SQL_TEXT_NOT_NULL ","
                             "lastExchanged " SQL_TEXT_NOT_NULL ", unreadMessages " SQL_INTEGER ", lastMessage  " SQL_TEXT ","
                             "lastOnline " SQL_TEXT ", activity " SQL_TEXT ", status " SQL_TEXT ", mood " SQL_TEXT ")"));
    execQuery(query,
              QStringLiteral("INSERT INTO Roster SELECT jid, name, lastExchanged, unreadMessages,"
                             "lastMessage, lastOnline, activity, status, mood FROM Roster_backup"));
    execQuery(query, QStringLiteral("DROP TABLE Roster_backup"));
    d->version = 4;
}

void Database::convertDatabaseToV5()
{
    DATABASE_CONVERT_TO_VERSION(4);
    QSqlQuery query(currentDatabase());
    execQuery(query, QStringLiteral("ALTER TABLE Messages ADD type " SQL_INTEGER));
    execQuery(query, QStringLiteral("UPDATE Messages SET type = 0 WHERE type IS NULL"));
    execQuery(query, QStringLiteral("ALTER TABLE Messages ADD mediaUrl " SQL_TEXT));
    d->version = 5;
}

void Database::convertDatabaseToV6()
{
    DATABASE_CONVERT_TO_VERSION(5);
    QSqlQuery query(currentDatabase());
    const QStringView newColumns[]{u"mediaSize " SQL_INTEGER, u"mediaContentType " SQL_TEXT, u"mediaLastModified " SQL_INTEGER, u"mediaLocation " SQL_TEXT};
    for (auto column : newColumns) {
        execQuery(query, u"ALTER TABLE Messages ADD " % column);
    }
    d->version = 6;
}

void Database::convertDatabaseToV7()
{
    DATABASE_CONVERT_TO_VERSION(6);
    QSqlQuery query(currentDatabase());
    execQuery(query, QStringLiteral("ALTER TABLE Messages ADD mediaThumb " SQL_BLOB));
    execQuery(query, QStringLiteral("ALTER TABLE Messages ADD mediaHashes " SQL_TEXT));
    d->version = 7;
}

void Database::convertDatabaseToV8()
{
    DATABASE_CONVERT_TO_VERSION(7);
    QSqlQuery query(currentDatabase());
    execQuery(query, QStringLiteral("CREATE TEMPORARY TABLE roster_backup(jid, name, lastExchanged, unreadMessages, lastMessage)"));
    execQuery(query, QStringLiteral("INSERT INTO roster_backup SELECT jid, name, lastExchanged, unreadMessages, lastMessage FROM Roster"));
    execQuery(query, QStringLiteral("DROP TABLE Roster"));
    execQuery(query,
              SQL_CREATE_TABLE("Roster",
                               SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(name, SQL_TEXT) SQL_ATTRIBUTE(lastExchanged, SQL_TEXT_NOT_NULL)
                                   SQL_ATTRIBUTE(unreadMessages, SQL_INTEGER) SQL_LAST_ATTRIBUTE(lastMessage, SQL_TEXT_NOT_NULL)));

    execQuery(query, QStringLiteral("INSERT INTO Roster SELECT jid, name, lastExchanged, unreadMessages, lastMessage FROM Roster_backup"));
    execQuery(query, QStringLiteral("DROP TABLE roster_backup"));
    d->version = 8;
}

void Database::convertDatabaseToV9()
{
    DATABASE_CONVERT_TO_VERSION(8);
    QSqlQuery query(currentDatabase());
    execQuery(query, QStringLiteral("ALTER TABLE Messages ADD edited " SQL_BOOL));
    d->version = 9;
}

void Database::convertDatabaseToV10()
{
    DATABASE_CONVERT_TO_VERSION(9);
    QSqlQuery query(currentDatabase());
    execQuery(query, QStringLiteral("ALTER TABLE Messages ADD isSpoiler " SQL_BOOL));
    execQuery(query, QStringLiteral("ALTER TABLE Messages ADD spoilerHint " SQL_TEXT));
    d->version = 10;
}

void Database::convertDatabaseToV11()
{
    DATABASE_CONVERT_TO_VERSION(10);
    QSqlQuery query(currentDatabase());
    execQuery(query, QStringLiteral("ALTER TABLE Messages ADD deliveryState " SQL_INTEGER));
    execQuery(query, QStringLiteral("UPDATE Messages SET deliveryState = 2 WHERE isDelivered = 1"));
    execQuery(query, QStringLiteral("UPDATE Messages SET deliveryState = 1 WHERE deliveryState IS NULL"));
    execQuery(query, QStringLiteral("ALTER TABLE Messages ADD errorText " SQL_TEXT));
    d->version = 11;
}

void Database::convertDatabaseToV12()
{
    DATABASE_CONVERT_TO_VERSION(11);
    QSqlQuery query(currentDatabase());
    execQuery(query, QStringLiteral("ALTER TABLE Messages ADD replaceId " SQL_TEXT));
    d->version = 12;
}

void Database::convertDatabaseToV13()
{
    DATABASE_CONVERT_TO_VERSION(12);
    QSqlQuery query(currentDatabase());
    execQuery(query, QStringLiteral("ALTER TABLE Messages ADD stanzaId " SQL_TEXT));
    execQuery(query, QStringLiteral("ALTER TABLE Messages ADD originId " SQL_TEXT));
    d->version = 13;
}

void Database::convertDatabaseToV14()
{
    DATABASE_CONVERT_TO_VERSION(13);
    QSqlQuery query(currentDatabase());
    execQuery(query, QStringLiteral("ALTER TABLE Roster ADD subscription " SQL_INTEGER));
    execQuery(query, QStringLiteral("ALTER TABLE Roster ADD encryption " SQL_INTEGER));
    execQuery(query, QStringLiteral("ALTER TABLE Messages ADD encryption " SQL_INTEGER));
    execQuery(query, QStringLiteral("ALTER TABLE Messages ADD senderKey " SQL_BLOB));
    d->version = 14;
}

void Database::convertDatabaseToV15()
{
    DATABASE_CONVERT_TO_VERSION(14);
    QSqlQuery query(currentDatabase());
    execQuery(query,
              SQL_CREATE_TABLE("trust_security_policies",
                               SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(encryption, SQL_TEXT_NOT_NULL)
                                   SQL_ATTRIBUTE(security_policy, SQL_INTEGER_NOT_NULL) "PRIMARY KEY(account, encryption)"));
    execQuery(query,
              SQL_CREATE_TABLE("trust_own_keys",
                               SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(encryption, SQL_TEXT_NOT_NULL)
                                   SQL_ATTRIBUTE(key_id, SQL_BLOB_NOT_NULL) "PRIMARY KEY(account, encryption)"));
    execQuery(query,
              SQL_CREATE_TABLE("trust_keys",
                               SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(encryption, SQL_TEXT_NOT_NULL)
                                   SQL_ATTRIBUTE(owner_jid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(key_id, SQL_BLOB_NOT_NULL)
                                       SQL_ATTRIBUTE(trust_level, SQL_INTEGER_NOT_NULL) "PRIMARY KEY(account, encryption, key_id, owner_jid)"));
    execQuery(query,
              SQL_CREATE_TABLE("trust_keys_unprocessed",
                               SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(encryption, SQL_TEXT_NOT_NULL)
                                   SQL_ATTRIBUTE(sender_key_id, SQL_BLOB_NOT_NULL) SQL_ATTRIBUTE(owner_jid, SQL_TEXT_NOT_NULL)
                                       SQL_ATTRIBUTE(key_id, SQL_BLOB_NOT_NULL)
                                           SQL_ATTRIBUTE(trust, SQL_BOOL_NOT_NULL) "PRIMARY KEY(account, encryption, key_id, owner_jid)"));
    d->version = 15;
}

void Database::convertDatabaseToV16()
{
    DATABASE_CONVERT_TO_VERSION(15);
    QSqlQuery query(currentDatabase());

    execQuery(query,
              SQL_CREATE_TABLE("omemo_devices_own",
                               SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(id, SQL_INTEGER_NOT_NULL) SQL_ATTRIBUTE(label, SQL_TEXT)
                                   SQL_ATTRIBUTE(private_key, SQL_BLOB) SQL_ATTRIBUTE(public_key, SQL_BLOB)
                                       SQL_ATTRIBUTE(latest_signed_pre_key_id, SQL_INTEGER_NOT_NULL)
                                           SQL_ATTRIBUTE(latest_pre_key_id, SQL_INTEGER_NOT_NULL) "PRIMARY KEY(account)"));
    execQuery(query,
              SQL_CREATE_TABLE("omemo_devices",
                               SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(user_jid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(id, SQL_INTEGER_NOT_NULL)
                                   SQL_ATTRIBUTE(label, SQL_TEXT) SQL_ATTRIBUTE(key_id, SQL_BLOB) SQL_ATTRIBUTE(session, SQL_BLOB)
                                       SQL_ATTRIBUTE(unresponded_stanzas_sent, SQL_INTEGER " DEFAULT 0")
                                           SQL_ATTRIBUTE(unresponded_stanzas_received, SQL_INTEGER " DEFAULT 0")
                                               SQL_ATTRIBUTE(removal_timestamp, SQL_INTEGER) "PRIMARY KEY(account, user_jid, id)"));
    execQuery(query,
              SQL_CREATE_TABLE("omemo_pre_key_pairs",
                               SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(id, SQL_BLOB_NOT_NULL)
                                   SQL_ATTRIBUTE(data, SQL_BLOB_NOT_NULL) "PRIMARY KEY(id)"));
    execQuery(query,
              SQL_CREATE_TABLE("omemo_pre_key_pairs_signed",
                               SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(id, SQL_BLOB_NOT_NULL) SQL_ATTRIBUTE(data, SQL_BLOB_NOT_NULL)
                                   SQL_ATTRIBUTE(creation_timestamp, SQL_INTEGER) "PRIMARY KEY(id)"));

    d->version = 16;
}

void Database::convertDatabaseToV17()
{
    DATABASE_CONVERT_TO_VERSION(16);
    QSqlQuery query(currentDatabase());
    execQuery(query, QStringLiteral("ALTER TABLE Roster ADD lastReadOwnMessageId " SQL_TEXT));
    execQuery(query, QStringLiteral("ALTER TABLE Roster ADD lastReadContactMessageId " SQL_TEXT));
    execQuery(query, QStringLiteral("ALTER TABLE Messages ADD isMarkable " SQL_BOOL));
    d->version = 17;
}

void Database::convertDatabaseToV18()
{
    DATABASE_CONVERT_TO_VERSION(17)
    QSqlQuery query(currentDatabase());

    // manually convert messages table
    execQuery(query,
              SQL_CREATE_TABLE("messages_tmp",
                               SQL_ATTRIBUTE(sender, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(recipient, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(timestamp, SQL_TEXT)
                                   SQL_ATTRIBUTE(message, SQL_TEXT) SQL_ATTRIBUTE(id, SQL_TEXT) SQL_ATTRIBUTE(encryption, SQL_INTEGER)
                                       SQL_ATTRIBUTE(senderKey, SQL_BLOB) SQL_ATTRIBUTE(deliveryState, SQL_INTEGER) SQL_ATTRIBUTE(isMarkable, SQL_BOOL)
                                           SQL_ATTRIBUTE(mediaUrl, SQL_TEXT) SQL_ATTRIBUTE(mediaLocation, SQL_TEXT) SQL_ATTRIBUTE(isEdited, SQL_BOOL)
                                               SQL_ATTRIBUTE(spoilerHint, SQL_TEXT) SQL_ATTRIBUTE(isSpoiler, SQL_BOOL) SQL_ATTRIBUTE(errorText, SQL_TEXT)
                                                   SQL_ATTRIBUTE(replaceId, SQL_TEXT) SQL_ATTRIBUTE(originId, SQL_TEXT) SQL_ATTRIBUTE(stanzaId, SQL_TEXT)
                                                       SQL_ATTRIBUTE(file_group_id, SQL_INTEGER) "FOREIGN KEY(sender) REFERENCES Roster (jid),"
                                                                                                 "FOREIGN KEY(recipient) REFERENCES Roster (jid)"));

    execQuery(query,
              QStringLiteral("INSERT INTO messages_tmp SELECT author, recipient, timestamp, message, id, encryption, "
                             "senderKey, deliveryState, isMarkable, mediaUrl, mediaLocation, edited, spoilerHint, "
                             "isSpoiler, errorText, replaceId, originId, stanzaId, NULL FROM Messages"));

    execQuery(query, QStringLiteral("DROP TABLE Messages"));

    execQuery(query,
              SQL_CREATE_TABLE("messages",
                               SQL_ATTRIBUTE(sender, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(recipient, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(timestamp, SQL_TEXT)
                                   SQL_ATTRIBUTE(message, SQL_TEXT) SQL_ATTRIBUTE(id, SQL_TEXT) SQL_ATTRIBUTE(encryption, SQL_INTEGER)
                                       SQL_ATTRIBUTE(senderKey, SQL_BLOB) SQL_ATTRIBUTE(deliveryState, SQL_INTEGER) SQL_ATTRIBUTE(isMarkable, SQL_BOOL)
                                           SQL_ATTRIBUTE(mediaUrl, SQL_TEXT) SQL_ATTRIBUTE(mediaLocation, SQL_TEXT) SQL_ATTRIBUTE(isEdited, SQL_BOOL)
                                               SQL_ATTRIBUTE(spoilerHint, SQL_TEXT) SQL_ATTRIBUTE(isSpoiler, SQL_BOOL) SQL_ATTRIBUTE(errorText, SQL_TEXT)
                                                   SQL_ATTRIBUTE(replaceId, SQL_TEXT) SQL_ATTRIBUTE(originId, SQL_TEXT) SQL_ATTRIBUTE(stanzaId, SQL_TEXT)
                                                       SQL_ATTRIBUTE(file_group_id, SQL_INTEGER) "FOREIGN KEY(sender) REFERENCES Roster (jid),"
                                                                                                 "FOREIGN KEY(recipient) REFERENCES Roster (jid)"));

    execQuery(query, QStringLiteral("INSERT INTO messages SELECT * FROM messages_tmp"));
    execQuery(query, QStringLiteral("DROP TABLE messages_tmp"));

    d->version = 18;
}

void Database::convertDatabaseToV19()
{
    DATABASE_CONVERT_TO_VERSION(18)
    QSqlQuery query(currentDatabase());

    // file sharing
    execQuery(query,
              SQL_CREATE_TABLE("files",
                               SQL_ATTRIBUTE(id, SQL_INTEGER_NOT_NULL) SQL_ATTRIBUTE(file_group_id, SQL_INTEGER_NOT_NULL) SQL_ATTRIBUTE(name, SQL_TEXT)
                                   SQL_ATTRIBUTE(description, SQL_TEXT) SQL_ATTRIBUTE(mime_type, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(size, SQL_INTEGER)
                                       SQL_ATTRIBUTE(last_modified, SQL_INTEGER_NOT_NULL) SQL_ATTRIBUTE(disposition, SQL_INTEGER_NOT_NULL)
                                           SQL_ATTRIBUTE(thumbnail, SQL_BLOB) SQL_ATTRIBUTE(local_file_path, SQL_TEXT) "PRIMARY KEY(id)"));
    execQuery(query,
              SQL_CREATE_TABLE("file_hashes",
                               SQL_ATTRIBUTE(data_id, SQL_INTEGER_NOT_NULL) SQL_ATTRIBUTE(hash_type, SQL_INTEGER_NOT_NULL)
                                   SQL_ATTRIBUTE(hash_value, SQL_BLOB_NOT_NULL) "PRIMARY KEY(data_id, hash_type)"));
    execQuery(query,
              SQL_CREATE_TABLE("file_http_sources", SQL_ATTRIBUTE(file_id, SQL_INTEGER_NOT_NULL) SQL_ATTRIBUTE(url, SQL_BLOB_NOT_NULL) "PRIMARY KEY(file_id)"));
    execQuery(query,
              SQL_CREATE_TABLE("file_encrypted_sources",
                               SQL_ATTRIBUTE(file_id, SQL_INTEGER_NOT_NULL) SQL_ATTRIBUTE(url, SQL_BLOB_NOT_NULL) SQL_ATTRIBUTE(cipher, SQL_INTEGER_NOT_NULL)
                                   SQL_ATTRIBUTE(key, SQL_BLOB_NOT_NULL) SQL_ATTRIBUTE(iv, SQL_BLOB_NOT_NULL)
                                       SQL_ATTRIBUTE(encrypted_data_id, SQL_INTEGER) "PRIMARY KEY(file_id)"));

    // manually convert messages table
    execQuery(query,
              SQL_CREATE_TABLE("messages_tmp",
                               SQL_ATTRIBUTE(sender, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(recipient, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(timestamp, SQL_TEXT)
                                   SQL_ATTRIBUTE(message, SQL_TEXT) SQL_ATTRIBUTE(id, SQL_TEXT) SQL_ATTRIBUTE(encryption, SQL_INTEGER)
                                       SQL_ATTRIBUTE(senderKey, SQL_BLOB) SQL_ATTRIBUTE(deliveryState, SQL_INTEGER) SQL_ATTRIBUTE(isMarkable, SQL_BOOL)
                                           SQL_ATTRIBUTE(isEdited, SQL_BOOL) SQL_ATTRIBUTE(spoilerHint, SQL_TEXT) SQL_ATTRIBUTE(isSpoiler, SQL_BOOL)
                                               SQL_ATTRIBUTE(errorText, SQL_TEXT) SQL_ATTRIBUTE(replaceId, SQL_TEXT) SQL_ATTRIBUTE(originId, SQL_TEXT)
                                                   SQL_ATTRIBUTE(stanzaId, SQL_TEXT)
                                                       SQL_ATTRIBUTE(file_group_id, SQL_INTEGER) "FOREIGN KEY(sender) REFERENCES Roster (jid),"
                                                                                                 "FOREIGN KEY(recipient) REFERENCES Roster (jid)"));

    execQuery(query,
              QStringLiteral("INSERT INTO messages_tmp SELECT sender, recipient, timestamp, message, id, encryption, "
                             "senderKey, deliveryState, isMarkable, isEdited, spoilerHint, isSpoiler, errorText, "
                             "replaceId, originId, stanzaId, file_group_id FROM messages"));

    execQuery(query, QStringLiteral("DROP TABLE messages"));

    execQuery(query,
              SQL_CREATE_TABLE("messages",
                               SQL_ATTRIBUTE(sender, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(recipient, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(timestamp, SQL_TEXT)
                                   SQL_ATTRIBUTE(message, SQL_TEXT) SQL_ATTRIBUTE(id, SQL_TEXT) SQL_ATTRIBUTE(encryption, SQL_INTEGER)
                                       SQL_ATTRIBUTE(senderKey, SQL_BLOB) SQL_ATTRIBUTE(deliveryState, SQL_INTEGER) SQL_ATTRIBUTE(isMarkable, SQL_BOOL)
                                           SQL_ATTRIBUTE(isEdited, SQL_BOOL) SQL_ATTRIBUTE(spoilerHint, SQL_TEXT) SQL_ATTRIBUTE(isSpoiler, SQL_BOOL)
                                               SQL_ATTRIBUTE(errorText, SQL_TEXT) SQL_ATTRIBUTE(replaceId, SQL_TEXT) SQL_ATTRIBUTE(originId, SQL_TEXT)
                                                   SQL_ATTRIBUTE(stanzaId, SQL_TEXT)
                                                       SQL_ATTRIBUTE(file_group_id, SQL_INTEGER) "FOREIGN KEY(sender) REFERENCES Roster (jid),"
                                                                                                 "FOREIGN KEY(recipient) REFERENCES Roster (jid)"));

    execQuery(query, QStringLiteral("INSERT INTO messages SELECT * FROM messages_tmp"));
    execQuery(query, QStringLiteral("DROP TABLE messages_tmp"));

    d->version = 19;
}

void Database::convertDatabaseToV20()
{
    DATABASE_CONVERT_TO_VERSION(19);
    QSqlQuery query(currentDatabase());
    execQuery(query, QStringLiteral("ALTER TABLE Roster ADD readMarkerPending " SQL_BOOL));
    d->version = 20;
}

void Database::convertDatabaseToV21()
{
    DATABASE_CONVERT_TO_VERSION(20);
    QSqlQuery query(currentDatabase());
    execQuery(query, QStringLiteral("ALTER TABLE Roster ADD pinningPosition " SQL_INTEGER));
    d->version = 21;
}

void Database::convertDatabaseToV22()
{
    DATABASE_CONVERT_TO_VERSION(21);
    QSqlQuery query(currentDatabase());

    execQuery(query,
              SQL_CREATE_TABLE(
                  "messageReactions",
                  SQL_ATTRIBUTE(messageSender, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(messageRecipient, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(messageId, SQL_TEXT_NOT_NULL)
                      SQL_ATTRIBUTE(senderJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(timestamp, SQL_INTEGER)
                          SQL_ATTRIBUTE(emoji, SQL_TEXT_NOT_NULL) "PRIMARY KEY(messageSender, messageRecipient, messageId, senderJid, timestamp, emoji)"));

    d->version = 22;
}

void Database::convertDatabaseToV23()
{
    DATABASE_CONVERT_TO_VERSION(22)
    QSqlQuery query(currentDatabase());

    // Rename the table "Roster" to "roster".
    // Remove the unused columns "lastExchanged" and "lastMessage".
    execQuery(query,
              SQL_CREATE_TABLE("roster_tmp",
                               SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(name, SQL_TEXT) SQL_ATTRIBUTE(subscription, SQL_INTEGER)
                                   SQL_ATTRIBUTE(encryption, SQL_INTEGER) SQL_ATTRIBUTE(unreadMessages, SQL_INTEGER)
                                       SQL_ATTRIBUTE(lastReadOwnMessageId, SQL_TEXT) SQL_ATTRIBUTE(lastReadContactMessageId, SQL_TEXT)
                                           SQL_ATTRIBUTE(readMarkerPending, SQL_BOOL) SQL_LAST_ATTRIBUTE(pinningPosition, SQL_INTEGER)));

    execQuery(query,
              QStringLiteral("INSERT INTO roster_tmp SELECT jid, name, subscription, encryption, unreadMessages, "
                             "lastReadOwnMessageId, lastReadContactMessageId, readMarkerPending, pinningPosition "
                             " FROM Roster"));

    execQuery(query, QStringLiteral("DROP TABLE Roster"));

    execQuery(query,
              SQL_CREATE_TABLE("roster",
                               SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(name, SQL_TEXT) SQL_ATTRIBUTE(subscription, SQL_INTEGER)
                                   SQL_ATTRIBUTE(encryption, SQL_INTEGER) SQL_ATTRIBUTE(unreadMessages, SQL_INTEGER)
                                       SQL_ATTRIBUTE(lastReadOwnMessageId, SQL_TEXT) SQL_ATTRIBUTE(lastReadContactMessageId, SQL_TEXT)
                                           SQL_ATTRIBUTE(readMarkerPending, SQL_BOOL) SQL_LAST_ATTRIBUTE(pinningPosition, SQL_INTEGER)));

    execQuery(query, QStringLiteral("INSERT INTO roster SELECT * FROM roster_tmp"));
    execQuery(query, QStringLiteral("DROP TABLE roster_tmp"));

    // Adapt the foreign keys of table "messages" to new table name "roster".
    execQuery(query,
              SQL_CREATE_TABLE("messages_tmp",
                               SQL_ATTRIBUTE(sender, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(recipient, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(timestamp, SQL_TEXT)
                                   SQL_ATTRIBUTE(message, SQL_TEXT) SQL_ATTRIBUTE(id, SQL_TEXT) SQL_ATTRIBUTE(encryption, SQL_INTEGER)
                                       SQL_ATTRIBUTE(senderKey, SQL_BLOB) SQL_ATTRIBUTE(deliveryState, SQL_INTEGER) SQL_ATTRIBUTE(isMarkable, SQL_BOOL)
                                           SQL_ATTRIBUTE(isEdited, SQL_BOOL) SQL_ATTRIBUTE(spoilerHint, SQL_TEXT) SQL_ATTRIBUTE(isSpoiler, SQL_BOOL)
                                               SQL_ATTRIBUTE(errorText, SQL_TEXT) SQL_ATTRIBUTE(replaceId, SQL_TEXT) SQL_ATTRIBUTE(originId, SQL_TEXT)
                                                   SQL_ATTRIBUTE(stanzaId, SQL_TEXT)
                                                       SQL_ATTRIBUTE(file_group_id, SQL_INTEGER) "FOREIGN KEY(sender) REFERENCES roster (jid),"
                                                                                                 "FOREIGN KEY(recipient) REFERENCES roster (jid)"));

    execQuery(query,
              QStringLiteral("INSERT INTO messages_tmp SELECT sender, recipient, timestamp, message, id, encryption, "
                             "senderKey, deliveryState, isMarkable, isEdited, spoilerHint, isSpoiler, errorText, "
                             "replaceId, originId, stanzaId, file_group_id FROM messages"));

    execQuery(query, QStringLiteral("DROP TABLE messages"));

    execQuery(query,
              SQL_CREATE_TABLE("messages",
                               SQL_ATTRIBUTE(sender, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(recipient, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(timestamp, SQL_TEXT)
                                   SQL_ATTRIBUTE(message, SQL_TEXT) SQL_ATTRIBUTE(id, SQL_TEXT) SQL_ATTRIBUTE(encryption, SQL_INTEGER)
                                       SQL_ATTRIBUTE(senderKey, SQL_BLOB) SQL_ATTRIBUTE(deliveryState, SQL_INTEGER) SQL_ATTRIBUTE(isMarkable, SQL_BOOL)
                                           SQL_ATTRIBUTE(isEdited, SQL_BOOL) SQL_ATTRIBUTE(spoilerHint, SQL_TEXT) SQL_ATTRIBUTE(isSpoiler, SQL_BOOL)
                                               SQL_ATTRIBUTE(errorText, SQL_TEXT) SQL_ATTRIBUTE(replaceId, SQL_TEXT) SQL_ATTRIBUTE(originId, SQL_TEXT)
                                                   SQL_ATTRIBUTE(stanzaId, SQL_TEXT)
                                                       SQL_ATTRIBUTE(file_group_id, SQL_INTEGER) "FOREIGN KEY(sender) REFERENCES roster (jid),"
                                                                                                 "FOREIGN KEY(recipient) REFERENCES roster (jid)"));

    execQuery(query, QStringLiteral("INSERT INTO messages SELECT * FROM messages_tmp"));
    execQuery(query, QStringLiteral("DROP TABLE messages_tmp"));

    // Use camelCase for all tables.

    execQuery(query, QStringLiteral("ALTER TABLE messages RENAME COLUMN file_group_id TO fileGroupId"));

    execQuery(query, QStringLiteral("ALTER TABLE files RENAME COLUMN file_group_id TO fileGroupId"));
    execQuery(query, QStringLiteral("ALTER TABLE files RENAME COLUMN mime_type TO mimeType"));
    execQuery(query, QStringLiteral("ALTER TABLE files RENAME COLUMN last_modified TO lastModified"));
    execQuery(query, QStringLiteral("ALTER TABLE files RENAME COLUMN local_file_path TO localFilePath"));

    execQuery(query, QStringLiteral("ALTER TABLE file_hashes RENAME TO fileHashes"));
    execQuery(query, QStringLiteral("ALTER TABLE fileHashes RENAME COLUMN data_id TO dataId"));
    execQuery(query, QStringLiteral("ALTER TABLE fileHashes RENAME COLUMN hash_type TO hashType"));
    execQuery(query, QStringLiteral("ALTER TABLE fileHashes RENAME COLUMN hash_value TO hashValue"));

    execQuery(query, QStringLiteral("ALTER TABLE file_http_sources RENAME TO fileHttpSources"));
    execQuery(query, QStringLiteral("ALTER TABLE fileHttpSources RENAME COLUMN file_id TO fileId"));

    execQuery(query, QStringLiteral("ALTER TABLE file_encrypted_sources RENAME TO fileEncryptedSources"));
    execQuery(query, QStringLiteral("ALTER TABLE fileEncryptedSources RENAME COLUMN file_id TO fileId"));
    execQuery(query, QStringLiteral("ALTER TABLE fileEncryptedSources RENAME COLUMN encrypted_data_id TO encryptedDataId"));

    execQuery(query, QStringLiteral("ALTER TABLE trust_security_policies RENAME TO trustSecurityPolicies"));
    execQuery(query, QStringLiteral("ALTER TABLE trustSecurityPolicies RENAME COLUMN security_policy TO securityPolicy"));

    execQuery(query, QStringLiteral("ALTER TABLE trust_own_keys RENAME TO trustOwnKeys"));
    execQuery(query, QStringLiteral("ALTER TABLE trustOwnKeys RENAME COLUMN key_id TO keyId"));

    execQuery(query, QStringLiteral("ALTER TABLE trust_keys RENAME TO trustKeys"));
    execQuery(query, QStringLiteral("ALTER TABLE trustKeys RENAME COLUMN owner_jid TO ownerJid"));
    execQuery(query, QStringLiteral("ALTER TABLE trustKeys RENAME COLUMN key_id TO keyId"));
    execQuery(query, QStringLiteral("ALTER TABLE trustKeys RENAME COLUMN trust_level TO trustLevel"));

    execQuery(query, QStringLiteral("ALTER TABLE trust_keys_unprocessed RENAME TO trustKeysUnprocessed"));
    execQuery(query, QStringLiteral("ALTER TABLE trustKeysUnprocessed RENAME COLUMN sender_key_id TO senderKeyId"));
    execQuery(query, QStringLiteral("ALTER TABLE trustKeysUnprocessed RENAME COLUMN owner_jid TO ownerJid"));
    execQuery(query, QStringLiteral("ALTER TABLE trustKeysUnprocessed RENAME COLUMN key_id TO keyId"));

    execQuery(query, QStringLiteral("ALTER TABLE omemo_devices_own RENAME TO omemoDevicesOwn"));
    execQuery(query, QStringLiteral("ALTER TABLE omemoDevicesOwn RENAME COLUMN private_key TO privateKey"));
    execQuery(query, QStringLiteral("ALTER TABLE omemoDevicesOwn RENAME COLUMN public_key TO publicKey"));
    execQuery(query, QStringLiteral("ALTER TABLE omemoDevicesOwn RENAME COLUMN latest_signed_pre_key_id TO latestSignedPreKeyId"));
    execQuery(query, QStringLiteral("ALTER TABLE omemoDevicesOwn RENAME COLUMN latest_pre_key_id TO latestPreKeyId"));

    execQuery(query, QStringLiteral("ALTER TABLE omemo_devices RENAME TO omemoDevices"));
    execQuery(query, QStringLiteral("ALTER TABLE omemoDevices RENAME COLUMN user_jid TO userJid"));
    execQuery(query, QStringLiteral("ALTER TABLE omemoDevices RENAME COLUMN key_id TO keyId"));
    execQuery(query, QStringLiteral("ALTER TABLE omemoDevices RENAME COLUMN unresponded_stanzas_sent TO unrespondedStanzasSent"));
    execQuery(query, QStringLiteral("ALTER TABLE omemoDevices RENAME COLUMN unresponded_stanzas_received TO unrespondedStanzasReceived"));
    execQuery(query, QStringLiteral("ALTER TABLE omemoDevices RENAME COLUMN removal_timestamp TO removalTimestamp"));

    execQuery(query, QStringLiteral("ALTER TABLE omemo_pre_key_pairs RENAME TO omemoPreKeyPairs"));

    execQuery(query, QStringLiteral("ALTER TABLE omemo_pre_key_pairs_signed RENAME TO omemoPreKeyPairsSigned"));
    execQuery(query, QStringLiteral("ALTER TABLE omemoPreKeyPairsSigned RENAME COLUMN creation_timestamp TO creationTimestamp"));

    d->version = 23;
}

void Database::convertDatabaseToV24()
{
    DATABASE_CONVERT_TO_VERSION(23);
    QSqlQuery query(currentDatabase());
    execQuery(query, QStringLiteral("ALTER TABLE Roster ADD chatStateSendingEnabled " SQL_BOOL));
    execQuery(query, QStringLiteral("ALTER TABLE Roster ADD readMarkerSendingEnabled " SQL_BOOL));
    d->version = 24;
}

void Database::convertDatabaseToV25()
{
    DATABASE_CONVERT_TO_VERSION(24);
    QSqlQuery query(currentDatabase());

    // Remove the column "isMarkable".
    execQuery(query,
              SQL_CREATE_TABLE("messages_tmp",
                               SQL_ATTRIBUTE(sender, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(recipient, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(timestamp, SQL_TEXT)
                                   SQL_ATTRIBUTE(message, SQL_TEXT) SQL_ATTRIBUTE(id, SQL_TEXT) SQL_ATTRIBUTE(encryption, SQL_INTEGER)
                                       SQL_ATTRIBUTE(senderKey, SQL_BLOB) SQL_ATTRIBUTE(deliveryState, SQL_INTEGER) SQL_ATTRIBUTE(isEdited, SQL_BOOL)
                                           SQL_ATTRIBUTE(spoilerHint, SQL_TEXT) SQL_ATTRIBUTE(isSpoiler, SQL_BOOL) SQL_ATTRIBUTE(errorText, SQL_TEXT)
                                               SQL_ATTRIBUTE(replaceId, SQL_TEXT) SQL_ATTRIBUTE(originId, SQL_TEXT) SQL_ATTRIBUTE(stanzaId, SQL_TEXT)
                                                   SQL_ATTRIBUTE(fileGroupId, SQL_INTEGER) "FOREIGN KEY(sender) REFERENCES roster (jid),"
                                                                                           "FOREIGN KEY(recipient) REFERENCES roster (jid)"));

    execQuery(query,
              QStringLiteral("INSERT INTO messages_tmp SELECT sender, recipient, timestamp, message, id, encryption, "
                             "senderKey, deliveryState, isEdited, spoilerHint, isSpoiler, errorText, replaceId, "
                             "originId, stanzaId, fileGroupId FROM messages"));

    execQuery(query, QStringLiteral("DROP TABLE messages"));

    execQuery(query,
              SQL_CREATE_TABLE("messages",
                               SQL_ATTRIBUTE(sender, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(recipient, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(timestamp, SQL_TEXT)
                                   SQL_ATTRIBUTE(message, SQL_TEXT) SQL_ATTRIBUTE(id, SQL_TEXT) SQL_ATTRIBUTE(encryption, SQL_INTEGER)
                                       SQL_ATTRIBUTE(senderKey, SQL_BLOB) SQL_ATTRIBUTE(deliveryState, SQL_INTEGER) SQL_ATTRIBUTE(isEdited, SQL_BOOL)
                                           SQL_ATTRIBUTE(spoilerHint, SQL_TEXT) SQL_ATTRIBUTE(isSpoiler, SQL_BOOL) SQL_ATTRIBUTE(errorText, SQL_TEXT)
                                               SQL_ATTRIBUTE(replaceId, SQL_TEXT) SQL_ATTRIBUTE(originId, SQL_TEXT) SQL_ATTRIBUTE(stanzaId, SQL_TEXT)
                                                   SQL_ATTRIBUTE(fileGroupId, SQL_INTEGER) "FOREIGN KEY(sender) REFERENCES roster (jid),"
                                                                                           "FOREIGN KEY(recipient) REFERENCES roster (jid)"));

    execQuery(query, QStringLiteral("INSERT INTO messages SELECT * FROM messages_tmp"));
    execQuery(query, QStringLiteral("DROP TABLE messages_tmp"));

    d->version = 25;
}

void Database::convertDatabaseToV26()
{
    DATABASE_CONVERT_TO_VERSION(25)
    QSqlQuery query(currentDatabase());

    // Make the column "pinningPosition" NOT NULL and set existing rows to -1.
    execQuery(query,
              SQL_CREATE_TABLE("roster_tmp",
                               SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(name, SQL_TEXT) SQL_ATTRIBUTE(subscription, SQL_INTEGER)
                                   SQL_ATTRIBUTE(encryption, SQL_INTEGER) SQL_ATTRIBUTE(unreadMessages, SQL_INTEGER)
                                       SQL_ATTRIBUTE(lastReadOwnMessageId, SQL_TEXT) SQL_ATTRIBUTE(lastReadContactMessageId, SQL_TEXT)
                                           SQL_ATTRIBUTE(readMarkerPending, SQL_BOOL) SQL_ATTRIBUTE(pinningPosition, SQL_INTEGER)
                                               SQL_ATTRIBUTE(chatStateSendingEnabled, SQL_BOOL) SQL_LAST_ATTRIBUTE(readMarkerSendingEnabled, SQL_BOOL)));

    execQuery(query,
              QStringLiteral("INSERT INTO roster_tmp SELECT jid, name, subscription, encryption, unreadMessages, "
                             "lastReadOwnMessageId, lastReadContactMessageId, readMarkerPending, pinningPosition, "
                             "chatStateSendingEnabled, readMarkerSendingEnabled FROM roster"));

    execQuery(query, QStringLiteral("UPDATE roster_tmp SET pinningPosition = -1 WHERE pinningPosition IS NULL"));
    execQuery(query, QStringLiteral("DROP TABLE roster"));

    execQuery(query,
              SQL_CREATE_TABLE("roster",
                               SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(name, SQL_TEXT) SQL_ATTRIBUTE(subscription, SQL_INTEGER)
                                   SQL_ATTRIBUTE(encryption, SQL_INTEGER) SQL_ATTRIBUTE(unreadMessages, SQL_INTEGER)
                                       SQL_ATTRIBUTE(lastReadOwnMessageId, SQL_TEXT) SQL_ATTRIBUTE(lastReadContactMessageId, SQL_TEXT)
                                           SQL_ATTRIBUTE(readMarkerPending, SQL_BOOL) SQL_ATTRIBUTE(pinningPosition, SQL_INTEGER_NOT_NULL)
                                               SQL_ATTRIBUTE(chatStateSendingEnabled, SQL_BOOL) SQL_LAST_ATTRIBUTE(readMarkerSendingEnabled, SQL_BOOL)));

    execQuery(query, QStringLiteral("INSERT INTO roster SELECT * FROM roster_tmp"));
    execQuery(query, QStringLiteral("DROP TABLE roster_tmp"));

    d->version = 26;
}

void Database::convertDatabaseToV27()
{
    DATABASE_CONVERT_TO_VERSION(26);
    QSqlQuery query(currentDatabase());
    execQuery(query, QStringLiteral("ALTER TABLE roster ADD draftMessageId " SQL_TEXT " REFERENCES messages (id)"));
    execQuery(query, QStringLiteral("CREATE VIEW chatMessages AS SELECT * FROM messages WHERE deliveryState != 4"));
    execQuery(query, QStringLiteral("CREATE VIEW draftMessages AS SELECT * FROM messages WHERE deliveryState = 4"));
    d->version = 27;
}

void Database::convertDatabaseToV28()
{
    DATABASE_CONVERT_TO_VERSION(27);
    QSqlQuery query(currentDatabase());
    execQuery(query, QStringLiteral("ALTER TABLE roster ADD notificationsMuted " SQL_BOOL));
    d->version = 28;
}

void Database::convertDatabaseToV29()
{
    DATABASE_CONVERT_TO_VERSION(28);
    QSqlQuery query(currentDatabase());

    // Add the column "deliveryState" and remove the column "timestamp" from the primary key.
    execQuery(query,
              SQL_CREATE_TABLE("messageReactions_tmp",
                               SQL_ATTRIBUTE(messageSender, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(messageRecipient, SQL_TEXT_NOT_NULL)
                                   SQL_ATTRIBUTE(messageId, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(senderJid, SQL_TEXT_NOT_NULL)
                                       SQL_ATTRIBUTE(emoji, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(timestamp, SQL_INTEGER)
                                           SQL_ATTRIBUTE(deliverySvoid convertDatabaseToV48();
                                                         tate, SQL_INTEGER) "PRIMARY KEY(messageSender, messageRecipient, messageId, senderJid, emoji)"));

    execQuery(query,
              QStringLiteral("INSERT INTO messageReactions_tmp SELECT messageSender, messageRecipient, messageId, "
                             "senderJid, emoji, timestamp, NULL FROM messageReactions"));

    execQuery(query, QStringLiteral("DROP TABLE messageReactions"));

    execQuery(query,
              SQL_CREATE_TABLE(
                  "messageReactions",
                  SQL_ATTRIBUTE(messageSender, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(messageRecipient, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(messageId, SQL_TEXT_NOT_NULL)
                      SQL_ATTRIBUTE(senderJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(emoji, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(timestamp, SQL_INTEGER)
                          SQL_ATTRIBUTE(deliveryState, SQL_INTEGER) "PRIMARY KEY(messageSender, messageRecipient, messageId, senderJid, emoji)"));

    execQuery(query, QStringLiteral("INSERT INTO messageReactions SELECT * FROM messageReactions_tmp"));
    execQuery(query, QStringLiteral("DROP TABLE messageReactions_tmp"));

    d->version = 29;
}

void Database::convertDatabaseToV30()
{
    DATABASE_CONVERT_TO_VERSION(29);
    QSqlQuery query(currentDatabase());
    execQuery(query, QStringLiteral("ALTER TABLE messages ADD removed " SQL_BOOL_NOT_NULL " DEFAULT 0"));
    execQuery(query, QStringLiteral("DROP VIEW chatMessages"));
    execQuery(query, QStringLiteral("CREATE VIEW chatMessages AS SELECT * FROM messages WHERE deliveryState != 4 AND removed != 1"));
    d->version = 30;
}

void Database::convertDatabaseToV31()
{
    DATABASE_CONVERT_TO_VERSION(30);
    QSqlQuery query(currentDatabase());
    execQuery(query, QStringLiteral("ALTER TABLE messages RENAME COLUMN message TO body"));
    d->version = 31;
}

void Database::convertDatabaseToV32()
{
    DATABASE_CONVERT_TO_VERSION(31);
    QSqlQuery query(currentDatabase());

    // Remove the column "isEdited".
    execQuery(query,
              SQL_CREATE_TABLE("messages_tmp",
                               SQL_ATTRIBUTE(sender, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(recipient, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(timestamp, SQL_TEXT)
                                   SQL_ATTRIBUTE(body, SQL_TEXT) SQL_ATTRIBUTE(id, SQL_TEXT) SQL_ATTRIBUTE(encryption, SQL_INTEGER)
                                       SQL_ATTRIBUTE(senderKey, SQL_BLOB) SQL_ATTRIBUTE(deliveryState, SQL_INTEGER) SQL_ATTRIBUTE(spoilerHint, SQL_TEXT)
                                           SQL_ATTRIBUTE(isSpoiler, SQL_BOOL) SQL_ATTRIBUTE(errorText, SQL_TEXT) SQL_ATTRIBUTE(replaceId, SQL_TEXT)
                                               SQL_ATTRIBUTE(originId, SQL_TEXT) SQL_ATTRIBUTE(stanzaId, SQL_TEXT) SQL_ATTRIBUTE(fileGroupId, SQL_INTEGER)
                                                   SQL_ATTRIBUTE(removed, SQL_BOOL_NOT_NULL) "FOREIGN KEY(sender) REFERENCES roster (jid),"
                                                                                             "FOREIGN KEY(recipient) REFERENCES roster (jid)"));

    execQuery(query,
              QStringLiteral("INSERT INTO messages_tmp SELECT sender, recipient, timestamp, body, id, encryption, "
                             "senderKey, deliveryState, spoilerHint, isSpoiler, errorText, replaceId, "
                             "originId, stanzaId, fileGroupId, removed FROM messages"));

    execQuery(query, QStringLiteral("DROP TABLE messages"));

    execQuery(query,
              SQL_CREATE_TABLE("messages",
                               SQL_ATTRIBUTE(sender, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(recipient, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(timestamp, SQL_TEXT)
                                   SQL_ATTRIBUTE(body, SQL_TEXT) SQL_ATTRIBUTE(id, SQL_TEXT) SQL_ATTRIBUTE(encryption, SQL_INTEGER)
                                       SQL_ATTRIBUTE(senderKey, SQL_BLOB) SQL_ATTRIBUTE(deliveryState, SQL_INTEGER) SQL_ATTRIBUTE(spoilerHint, SQL_TEXT)
                                           SQL_ATTRIBUTE(isSpoiler, SQL_BOOL) SQL_ATTRIBUTE(errorText, SQL_TEXT) SQL_ATTRIBUTE(replaceId, SQL_TEXT)
                                               SQL_ATTRIBUTE(originId, SQL_TEXT) SQL_ATTRIBUTE(stanzaId, SQL_TEXT) SQL_ATTRIBUTE(fileGroupId, SQL_INTEGER)
                                                   SQL_ATTRIBUTE(removed, SQL_BOOL_NOT_NULL) "FOREIGN KEY(sender) REFERENCES roster (jid),"
                                                                                             "FOREIGN KEY(recipient) REFERENCES roster (jid)"));

    execQuery(query, QStringLiteral("INSERT INTO messages SELECT * FROM messages_tmp"));
    execQuery(query, QStringLiteral("DROP TABLE messages_tmp"));

    d->version = 32;
}

void Database::convertDatabaseToV33()
{
    DATABASE_CONVERT_TO_VERSION(32);
    QSqlQuery query(currentDatabase());

    // Add the column "accountJid" and set a new primary key.
    // The value for the new column "accountJid" cannot be determined by the database.
    // Thus, the table "roster" is removed and recreated in order to store the latest values from
    // the server (e.g., "name") in the database again and include the new "accountJid".
    // Unfortunately, all data not stored on the server (e.g., "encryption") is lost.
    execQuery(query, QStringLiteral("DROP TABLE roster"));
    execQuery(query,
              SQL_CREATE_TABLE("roster",
                               SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(name, SQL_TEXT)
                                   SQL_ATTRIBUTE(subscription, SQL_INTEGER) SQL_ATTRIBUTE(encryption, SQL_INTEGER) SQL_ATTRIBUTE(unreadMessages, SQL_INTEGER)
                                       SQL_ATTRIBUTE(lastReadOwnMessageId, SQL_TEXT) SQL_ATTRIBUTE(lastReadContactMessageId, SQL_TEXT)
                                           SQL_ATTRIBUTE(readMarkerPending, SQL_BOOL) SQL_ATTRIBUTE(pinningPosition, SQL_INTEGER_NOT_NULL)
                                               SQL_ATTRIBUTE(chatStateSendingEnabled, SQL_BOOL) SQL_ATTRIBUTE(readMarkerSendingEnabled, SQL_BOOL)
                                                   SQL_ATTRIBUTE(draftMessageId, SQL_TEXT)
                                                       SQL_ATTRIBUTE(notificationsMuted, SQL_BOOL) "PRIMARY KEY(accountJid, jid),"
                                                                                                   "FOREIGN KEY(draftMessageId) REFERENCES messages (id)"));

    d->version = 33;
}

void Database::convertDatabaseToV34()
{
    DATABASE_CONVERT_TO_VERSION(33);
    QSqlQuery query(currentDatabase());

    execQuery(query,
              SQL_CREATE_TABLE("rosterGroups",
                               SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(chatJid, SQL_TEXT_NOT_NULL)
                                   SQL_ATTRIBUTE(name, SQL_TEXT_NOT_NULL) "PRIMARY KEY(accountJid, chatJid, name),"
                                                                          "FOREIGN KEY(accountJid) REFERENCES roster (accountJid),"
                                                                          "FOREIGN KEY(chatJid) REFERENCES roster (jid)"));

    d->version = 34;
}

void Database::convertDatabaseToV35()
{
    DATABASE_CONVERT_TO_VERSION(34);
    QSqlQuery query(currentDatabase());

    // Remove the column "draftMessageId".
    execQuery(query,
              SQL_CREATE_TABLE("roster_tmp",
                               SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(name, SQL_TEXT)
                                   SQL_ATTRIBUTE(subscription, SQL_INTEGER) SQL_ATTRIBUTE(encryption, SQL_INTEGER) SQL_ATTRIBUTE(unreadMessages, SQL_INTEGER)
                                       SQL_ATTRIBUTE(lastReadOwnMessageId, SQL_TEXT) SQL_ATTRIBUTE(lastReadContactMessageId, SQL_TEXT)
                                           SQL_ATTRIBUTE(readMarkerPending, SQL_BOOL) SQL_ATTRIBUTE(pinningPosition, SQL_INTEGER_NOT_NULL)
                                               SQL_ATTRIBUTE(chatStateSendingEnabled, SQL_BOOL) SQL_ATTRIBUTE(readMarkerSendingEnabled, SQL_BOOL)
                                                   SQL_ATTRIBUTE(notificationsMuted, SQL_BOOL) "PRIMARY KEY(accountJid, jid)"));

    execQuery(query,
              QStringLiteral("INSERT INTO roster_tmp SELECT accountJid, jid, name, subscription, encryption, unreadMessages, "
                             "lastReadOwnMessageId, lastReadContactMessageId, readMarkerPending, pinningPosition, "
                             "chatStateSendingEnabled, readMarkerSendingEnabled, notificationsMuted FROM roster"));

    execQuery(query, QStringLiteral("DROP TABLE roster"));

    execQuery(query,
              SQL_CREATE_TABLE("roster",
                               SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(name, SQL_TEXT)
                                   SQL_ATTRIBUTE(subscription, SQL_INTEGER) SQL_ATTRIBUTE(encryption, SQL_INTEGER) SQL_ATTRIBUTE(unreadMessages, SQL_INTEGER)
                                       SQL_ATTRIBUTE(lastReadOwnMessageId, SQL_TEXT) SQL_ATTRIBUTE(lastReadContactMessageId, SQL_TEXT)
                                           SQL_ATTRIBUTE(readMarkerPending, SQL_BOOL) SQL_ATTRIBUTE(pinningPosition, SQL_INTEGER_NOT_NULL)
                                               SQL_ATTRIBUTE(chatStateSendingEnabled, SQL_BOOL) SQL_ATTRIBUTE(readMarkerSendingEnabled, SQL_BOOL)
                                                   SQL_ATTRIBUTE(notificationsMuted, SQL_BOOL) "PRIMARY KEY(accountJid, jid)"));

    execQuery(query, QStringLiteral("INSERT INTO roster SELECT * FROM roster_tmp"));
    execQuery(query, QStringLiteral("DROP TABLE roster_tmp"));

    d->version = 35;
}

void Database::convertDatabaseToV36()
{
    DATABASE_CONVERT_TO_VERSION(35);
    QSqlQuery query(currentDatabase());

    // Replace the columns "sender" and "recipient" with "accountJid", "chatJid" and "senderId".
    // Set new foreign keys.
    // The values for the new columns cannot be determined by the database.
    // Thus, the table "messages" is removed and recreated in order to store the latest values from
    // the server in the database again and include the values for the new columns.
    // Unfortunately, all data not stored on the server (e.g., "removed") is lost.
    execQuery(query, QStringLiteral("DROP TABLE messages"));
    execQuery(
        query,
        SQL_CREATE_TABLE("messages",
                         SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(chatJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(senderId, SQL_TEXT)
                             SQL_ATTRIBUTE(timestamp, SQL_TEXT) SQL_ATTRIBUTE(body, SQL_TEXT) SQL_ATTRIBUTE(id, SQL_TEXT) SQL_ATTRIBUTE(encryption, SQL_INTEGER)
                                 SQL_ATTRIBUTE(senderKey, SQL_BLOB) SQL_ATTRIBUTE(deliveryState, SQL_INTEGER) SQL_ATTRIBUTE(spoilerHint, SQL_TEXT)
                                     SQL_ATTRIBUTE(isSpoiler, SQL_BOOL) SQL_ATTRIBUTE(errorText, SQL_TEXT) SQL_ATTRIBUTE(replaceId, SQL_TEXT)
                                         SQL_ATTRIBUTE(originId, SQL_TEXT) SQL_ATTRIBUTE(stanzaId, SQL_TEXT) SQL_ATTRIBUTE(fileGroupId, SQL_INTEGER)
                                             SQL_ATTRIBUTE(removed, SQL_BOOL_NOT_NULL) "FOREIGN KEY(accountJid, chatJid) REFERENCES roster (accountJid, jid)"));

    // Replace the columns "messageSender" and "messageRecipient" with "accountJid", "chatJid" and
    // "senderId".
    // Set a new primary key accordingly.
    // The values for the new columns cannot be determined by the database.
    // Thus, the table "messageReactions" is removed and recreated in order to store the latest
    // values from the server in the database again and include the values for the new columns.
    // Unfortunately, all data not stored on the server (e.g., "deliveryState") is lost.
    execQuery(query, QStringLiteral("DROP TABLE messageReactions"));
    execQuery(query,
              SQL_CREATE_TABLE(
                  "messageReactions",
                  SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(chatJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(messageSenderId, SQL_TEXT_NOT_NULL)
                      SQL_ATTRIBUTE(messageId, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(senderJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(emoji, SQL_TEXT_NOT_NULL)
                          SQL_ATTRIBUTE(timestamp, SQL_INTEGER)
                              SQL_ATTRIBUTE(deliveryState, SQL_INTEGER) "PRIMARY KEY(accountJid, chatJid, messageSenderId, messageId, senderJid, emoji)"));

    d->version = 36;
}

void Database::convertDatabaseToV37()
{
    DATABASE_CONVERT_TO_VERSION(36);
    QSqlQuery query(currentDatabase());

    // Reorder various columns for a consistent order through the whole code base.
    execQuery(
        query,
        SQL_CREATE_TABLE("messages_tmp",
                         SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(chatJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(senderId, SQL_TEXT)
                             SQL_ATTRIBUTE(id, SQL_TEXT) SQL_ATTRIBUTE(originId, SQL_TEXT) SQL_ATTRIBUTE(stanzaId, SQL_TEXT) SQL_ATTRIBUTE(replaceId, SQL_TEXT)
                                 SQL_ATTRIBUTE(timestamp, SQL_TEXT) SQL_ATTRIBUTE(body, SQL_TEXT) SQL_ATTRIBUTE(encryption, SQL_INTEGER)
                                     SQL_ATTRIBUTE(senderKey, SQL_BLOB) SQL_ATTRIBUTE(deliveryState, SQL_INTEGER) SQL_ATTRIBUTE(isSpoiler, SQL_BOOL)
                                         SQL_ATTRIBUTE(spoilerHint, SQL_TEXT) SQL_ATTRIBUTE(fileGroupId, SQL_INTEGER) SQL_ATTRIBUTE(errorText, SQL_TEXT)
                                             SQL_ATTRIBUTE(removed, SQL_BOOL_NOT_NULL) "FOREIGN KEY(accountJid, chatJid) REFERENCES roster (accountJid, jid)"));

    execQuery(query,
              QStringLiteral("INSERT INTO messages_tmp SELECT accountJid, chatJid, senderId, id, originId, stanzaId, "
                             "replaceId, timestamp, body, encryption, senderKey, deliveryState, isSpoiler, spoilerHint, "
                             "fileGroupId, errorText, removed FROM messages"));

    execQuery(query, QStringLiteral("DROP TABLE messages"));
    execQuery(
        query,
        SQL_CREATE_TABLE("messages",
                         SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(chatJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(senderId, SQL_TEXT)
                             SQL_ATTRIBUTE(id, SQL_TEXT) SQL_ATTRIBUTE(originId, SQL_TEXT) SQL_ATTRIBUTE(stanzaId, SQL_TEXT) SQL_ATTRIBUTE(replaceId, SQL_TEXT)
                                 SQL_ATTRIBUTE(timestamp, SQL_TEXT) SQL_ATTRIBUTE(body, SQL_TEXT) SQL_ATTRIBUTE(encryption, SQL_INTEGER)
                                     SQL_ATTRIBUTE(senderKey, SQL_BLOB) SQL_ATTRIBUTE(deliveryState, SQL_INTEGER) SQL_ATTRIBUTE(isSpoiler, SQL_BOOL)
                                         SQL_ATTRIBUTE(spoilerHint, SQL_TEXT) SQL_ATTRIBUTE(fileGroupId, SQL_INTEGER) SQL_ATTRIBUTE(errorText, SQL_TEXT)
                                             SQL_ATTRIBUTE(removed, SQL_BOOL_NOT_NULL) "FOREIGN KEY(accountJid, chatJid) REFERENCES roster (accountJid, jid)"));

    execQuery(query, QStringLiteral("INSERT INTO messages SELECT * FROM messages_tmp"));
    execQuery(query, QStringLiteral("DROP TABLE messages_tmp"));

    d->version = 37;
}

void Database::convertDatabaseToV38()
{
    DATABASE_CONVERT_TO_VERSION(37);
    QSqlQuery query(currentDatabase());
    execQuery(query, QStringLiteral("ALTER TABLE Roster ADD automaticMediaDownloadsRule " SQL_INTEGER));
    d->version = 38;
}

void Database::convertDatabaseToV39()
{
    DATABASE_CONVERT_TO_VERSION(38)
    QSqlQuery query(currentDatabase());

    // blocked
    execQuery(query,
              SQL_CREATE_TABLE("blocked", SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL) "PRIMARY KEY(accountJid, jid)"));

    d->version = 39;
}

void Database::convertDatabaseToV40()
{
    DATABASE_CONVERT_TO_VERSION(39)
    QSqlQuery query(currentDatabase());

    execQuery(query,
              SQL_CREATE_TABLE("accounts",
                               SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(name, SQL_TEXT) SQL_ATTRIBUTE(latestMessageStanzaId, SQL_TEXT)
                                   SQL_ATTRIBUTE(latestMessageTimestamp, SQL_TEXT) "PRIMARY KEY(jid)"));

    d->version = 40;
}

void Database::convertDatabaseToV41()
{
    DATABASE_CONVERT_TO_VERSION(40)
    QSqlQuery query(currentDatabase());

    execQuery(query, QStringLiteral("ALTER TABLE files ADD externalId TEXT"));

    d->version = 41;
}

void Database::convertDatabaseToV42()
{
    DATABASE_CONVERT_TO_VERSION(41)
    QSqlQuery query(currentDatabase());
    execQuery(query, QStringLiteral("ALTER TABLE accounts ADD httpUploadLimit " SQL_INTEGER));
    d->version = 42;
}

void Database::convertDatabaseToV43()
{
    DATABASE_CONVERT_TO_VERSION(42)
    QSqlQuery query(currentDatabase());

    execQuery(query, QStringLiteral("ALTER TABLE accounts RENAME COLUMN latestMessageTimestamp TO latestMessageStanzaTimestamp"));

    // Add the columns "groupChatParticipantId", "groupChatFlags", "latestGroupChatMessageStanzaId",
    // "latestGroupChatMessageStanzaTimestamp", "groupChatName", "groupChatDescription".
    execQuery(query,
              SQL_CREATE_TABLE("roster_tmp",
                               SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(name, SQL_TEXT)
                                   SQL_ATTRIBUTE(subscription, SQL_INTEGER) SQL_ATTRIBUTE(groupChatParticipantId, SQL_TEXT)
                                       SQL_ATTRIBUTE(groupChatName, SQL_TEXT) SQL_ATTRIBUTE(groupChatDescription, SQL_TEXT)
                                           SQL_ATTRIBUTE(groupChatFlags, SQL_INTEGER) SQL_ATTRIBUTE(encryption, SQL_INTEGER)
                                               SQL_ATTRIBUTE(unreadMessages, SQL_INTEGER) SQL_ATTRIBUTE(lastReadOwnMessageId, SQL_TEXT)
                                                   SQL_ATTRIBUTE(lastReadContactMessageId, SQL_TEXT) SQL_ATTRIBUTE(latestGroupChatMessageStanzaId, SQL_TEXT)
                                                       SQL_ATTRIBUTE(latestGroupChatMessageStanzaTimestamp, SQL_TEXT) SQL_ATTRIBUTE(readMarkerPending, SQL_BOOL)
                                                           SQL_ATTRIBUTE(pinningPosition, SQL_INTEGER_NOT_NULL) SQL_ATTRIBUTE(chatStateSendingEnabled, SQL_BOOL)
                                                               SQL_ATTRIBUTE(readMarkerSendingEnabled, SQL_BOOL) SQL_ATTRIBUTE(notificationsMuted, SQL_BOOL)
                                                                   SQL_ATTRIBUTE(automaticMediaDownloadsRule, SQL_INTEGER) "PRIMARY KEY(accountJid, jid)"));

    execQuery(query,
              QStringLiteral("INSERT INTO roster_tmp SELECT accountJid, jid, name, subscription, NULL, NULL, NULL, "
                             "NULL, encryption, unreadMessages, lastReadOwnMessageId, lastReadContactMessageId, NULL, "
                             "NULL, readMarkerPending, pinningPosition, chatStateSendingEnabled, "
                             "readMarkerSendingEnabled, notificationsMuted, automaticMediaDownloadsRule FROM roster"));

    execQuery(query, QStringLiteral("DROP TABLE roster"));
    execQuery(query,
              SQL_CREATE_TABLE("roster",
                               SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(name, SQL_TEXT)
                                   SQL_ATTRIBUTE(subscription, SQL_INTEGER) SQL_ATTRIBUTE(groupChatParticipantId, SQL_TEXT)
                                       SQL_ATTRIBUTE(groupChatName, SQL_TEXT) SQL_ATTRIBUTE(groupChatDescription, SQL_TEXT)
                                           SQL_ATTRIBUTE(groupChatFlags, SQL_INTEGER) SQL_ATTRIBUTE(encryption, SQL_INTEGER)
                                               SQL_ATTRIBUTE(unreadMessages, SQL_INTEGER) SQL_ATTRIBUTE(lastReadOwnMessageId, SQL_TEXT)
                                                   SQL_ATTRIBUTE(lastReadContactMessageId, SQL_TEXT) SQL_ATTRIBUTE(latestGroupChatMessageStanzaId, SQL_TEXT)
                                                       SQL_ATTRIBUTE(latestGroupChatMessageStanzaTimestamp, SQL_TEXT) SQL_ATTRIBUTE(readMarkerPending, SQL_BOOL)
                                                           SQL_ATTRIBUTE(pinningPosition, SQL_INTEGER_NOT_NULL) SQL_ATTRIBUTE(chatStateSendingEnabled, SQL_BOOL)
                                                               SQL_ATTRIBUTE(readMarkerSendingEnabled, SQL_BOOL) SQL_ATTRIBUTE(notificationsMuted, SQL_BOOL)
                                                                   SQL_ATTRIBUTE(automaticMediaDownloadsRule, SQL_INTEGER) "PRIMARY KEY(accountJid, jid)"));

    execQuery(query, QStringLiteral("INSERT INTO roster SELECT * FROM roster_tmp"));
    execQuery(query, QStringLiteral("DROP TABLE roster_tmp"));

    // Add the columns "isOwn", "groupChatSenderId", "groupChatInviterJid", "groupChatInviteeJid",
    // "groupChatInvitationJid" and "groupChatToken".
    // Remove the column "senderId".
    execQuery(
        query,
        SQL_CREATE_TABLE(
            "messages_tmp",
            SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(chatJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(isOwn, SQL_BOOL)
                SQL_ATTRIBUTE(groupChatSenderId, SQL_TEXT) SQL_ATTRIBUTE(id, SQL_TEXT) SQL_ATTRIBUTE(originId, SQL_TEXT) SQL_ATTRIBUTE(stanzaId, SQL_TEXT)
                    SQL_ATTRIBUTE(replaceId, SQL_TEXT) SQL_ATTRIBUTE(timestamp, SQL_TEXT) SQL_ATTRIBUTE(body, SQL_TEXT) SQL_ATTRIBUTE(encryption, SQL_INTEGER)
                        SQL_ATTRIBUTE(senderKey, SQL_BLOB) SQL_ATTRIBUTE(deliveryState, SQL_INTEGER) SQL_ATTRIBUTE(isSpoiler, SQL_BOOL)
                            SQL_ATTRIBUTE(spoilerHint, SQL_TEXT) SQL_ATTRIBUTE(fileGroupId, SQL_INTEGER) SQL_ATTRIBUTE(groupChatInviterJid, SQL_TEXT)
                                SQL_ATTRIBUTE(groupChatInviteeJid, SQL_TEXT) SQL_ATTRIBUTE(groupChatInvitationJid, SQL_TEXT)
                                    SQL_ATTRIBUTE(groupChatToken, SQL_TEXT) SQL_ATTRIBUTE(errorText, SQL_TEXT)
                                        SQL_ATTRIBUTE(removed, SQL_BOOL_NOT_NULL) "FOREIGN KEY(accountJid, chatJid) REFERENCES roster (accountJid, jid)"));

    execQuery(query, QStringLiteral(R"(
			INSERT INTO messages_tmp
			SELECT accountJid, chatJid, 0, NULL, id, originId, stanzaId, replaceId, timestamp,
			body, encryption, senderKey, deliveryState, isSpoiler, spoilerHint, fileGroupId, NULL,
			NULL, NULL, NULL, errorText, removed
			FROM messages
			WHERE accountJid != senderId
		)"));
    execQuery(query, QStringLiteral(R"(
			INSERT INTO messages_tmp
			SELECT accountJid, chatJid, 1, NULL, id, originId, stanzaId, replaceId, timestamp,
			body, encryption, senderKey, deliveryState, isSpoiler, spoilerHint, fileGroupId, NULL,
			NULL, NULL, NULL, errorText, removed
			FROM messages
			WHERE accountJid = senderId
		)"));

    execQuery(query, QStringLiteral("DROP TABLE messages"));
    execQuery(
        query,
        SQL_CREATE_TABLE(
            "messages",
            SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(chatJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(isOwn, SQL_BOOL_NOT_NULL)
                SQL_ATTRIBUTE(groupChatSenderId, SQL_TEXT) SQL_ATTRIBUTE(id, SQL_TEXT) SQL_ATTRIBUTE(originId, SQL_TEXT) SQL_ATTRIBUTE(stanzaId, SQL_TEXT)
                    SQL_ATTRIBUTE(replaceId, SQL_TEXT) SQL_ATTRIBUTE(timestamp, SQL_TEXT) SQL_ATTRIBUTE(body, SQL_TEXT) SQL_ATTRIBUTE(encryption, SQL_INTEGER)
                        SQL_ATTRIBUTE(senderKey, SQL_BLOB) SQL_ATTRIBUTE(deliveryState, SQL_INTEGER) SQL_ATTRIBUTE(isSpoiler, SQL_BOOL)
                            SQL_ATTRIBUTE(spoilerHint, SQL_TEXT) SQL_ATTRIBUTE(fileGroupId, SQL_INTEGER) SQL_ATTRIBUTE(groupChatInviterJid, SQL_TEXT)
                                SQL_ATTRIBUTE(groupChatInviteeJid, SQL_TEXT) SQL_ATTRIBUTE(groupChatInvitationJid, SQL_TEXT)
                                    SQL_ATTRIBUTE(groupChatToken, SQL_TEXT) SQL_ATTRIBUTE(errorText, SQL_TEXT)
                                        SQL_ATTRIBUTE(removed, SQL_BOOL_NOT_NULL) "FOREIGN KEY(accountJid, chatJid) REFERENCES roster (accountJid, jid)"));

    execQuery(query, QStringLiteral("INSERT INTO messages SELECT * FROM messages_tmp"));
    execQuery(query, QStringLiteral("DROP TABLE messages_tmp"));

    // Rename the column "senderJid" to "senderId", make it nullable and update the primary key
    // accordingly.
    execQuery(query,
              SQL_CREATE_TABLE("messageReactions_tmp",
                               SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(chatJid, SQL_TEXT_NOT_NULL)
                                   SQL_ATTRIBUTE(messageSenderId, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(messageId, SQL_TEXT_NOT_NULL)
                                       SQL_ATTRIBUTE(senderId, SQL_TEXT) SQL_ATTRIBUTE(emoji, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(timestamp, SQL_INTEGER)
                                           SQL_ATTRIBUTE(deliveryState, SQL_INTEGER) "PRIMARY KEY(accountJid, chatJid, messageId, senderId, emoji)"));

    execQuery(query,
              QStringLiteral("INSERT INTO messageReactions_tmp SELECT accountJid, chatJid, messageSenderId, messageId, "
                             "senderJid, emoji, timestamp, deliveryState FROM messageReactions"));

    execQuery(query, QStringLiteral("DROP TABLE messageReactions"));
    execQuery(query,
              SQL_CREATE_TABLE("messageReactions",
                               SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(chatJid, SQL_TEXT_NOT_NULL)
                                   SQL_ATTRIBUTE(messageSenderId, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(messageId, SQL_TEXT_NOT_NULL)
                                       SQL_ATTRIBUTE(senderId, SQL_TEXT) SQL_ATTRIBUTE(emoji, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(timestamp, SQL_INTEGER)
                                           SQL_ATTRIBUTE(deliveryState, SQL_INTEGER) "PRIMARY KEY(accountJid, chatJid, messageId, senderId, emoji)"));

    execQuery(query, QStringLiteral("INSERT INTO messageReactions SELECT * FROM messageReactions_tmp"));
    execQuery(query, QStringLiteral("DROP TABLE messageReactions_tmp"));

    execQuery(query, QStringLiteral("UPDATE messageReactions SET senderId = NULL WHERE senderId = accountJid"));

    // Create the table "groupChatUsers".
    execQuery(query,
              SQL_CREATE_TABLE("groupChatUsers",
                               SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(chatJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(id, SQL_TEXT_NOT_NULL)
                                   SQL_ATTRIBUTE(jid, SQL_TEXT) SQL_ATTRIBUTE(name, SQL_TEXT)
                                       SQL_ATTRIBUTE(status, SQL_INTEGER) "PRIMARY KEY(accountJid, chatJid, id, jid),"
                                                                          "FOREIGN KEY(accountJid, chatJid) REFERENCES " DB_TABLE_ROSTER " (accountJid, jid)"));

    d->version = 43;
}

void Database::convertDatabaseToV44()
{
    DATABASE_CONVERT_TO_VERSION(43)
    QSqlQuery query(currentDatabase());

    execQuery(query, QStringLiteral("ALTER TABLE accounts ADD contactNotificationRule " SQL_INTEGER));
    execQuery(query, QStringLiteral("ALTER TABLE accounts ADD groupChatNotificationRule " SQL_INTEGER));

    // Replace the column "notificationsMuted" with "notificationRule".
    execQuery(query,
              SQL_CREATE_TABLE("roster_tmp",
                               SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(name, SQL_TEXT)
                                   SQL_ATTRIBUTE(subscription, SQL_INTEGER) SQL_ATTRIBUTE(groupChatParticipantId, SQL_TEXT)
                                       SQL_ATTRIBUTE(groupChatName, SQL_TEXT) SQL_ATTRIBUTE(groupChatDescription, SQL_TEXT)
                                           SQL_ATTRIBUTE(groupChatFlags, SQL_INTEGER) SQL_ATTRIBUTE(encryption, SQL_INTEGER)
                                               SQL_ATTRIBUTE(unreadMessages, SQL_INTEGER) SQL_ATTRIBUTE(lastReadOwnMessageId, SQL_TEXT)
                                                   SQL_ATTRIBUTE(lastReadContactMessageId, SQL_TEXT) SQL_ATTRIBUTE(latestGroupChatMessageStanzaId, SQL_TEXT)
                                                       SQL_ATTRIBUTE(latestGroupChatMessageStanzaTimestamp, SQL_TEXT) SQL_ATTRIBUTE(readMarkerPending, SQL_BOOL)
                                                           SQL_ATTRIBUTE(pinningPosition, SQL_INTEGER_NOT_NULL) SQL_ATTRIBUTE(chatStateSendingEnabled, SQL_BOOL)
                                                               SQL_ATTRIBUTE(readMarkerSendingEnabled, SQL_BOOL) SQL_ATTRIBUTE(notificationRule, SQL_INTEGER)
                                                                   SQL_ATTRIBUTE(automaticMediaDownloadsRule, SQL_INTEGER) "PRIMARY KEY(accountJid, jid)"));

    execQuery(query, QStringLiteral(R"(
			INSERT INTO roster_tmp
			SELECT accountJid, jid, name, subscription, groupChatParticipantId, groupChatName,
			groupChatDescription, groupChatFlags, encryption, unreadMessages, lastReadOwnMessageId,
			lastReadContactMessageId, latestGroupChatMessageStanzaId,
			latestGroupChatMessageStanzaTimestamp, readMarkerPending, pinningPosition,
			chatStateSendingEnabled, readMarkerSendingEnabled, 1,
			automaticMediaDownloadsRule
			FROM roster
			WHERE notificationsMuted
		)"));
    execQuery(query, QStringLiteral(R"(
			INSERT INTO roster_tmp
			SELECT accountJid, jid, name, subscription, groupChatParticipantId, groupChatName,
			groupChatDescription, groupChatFlags, encryption, unreadMessages, lastReadOwnMessageId,
			lastReadContactMessageId, latestGroupChatMessageStanzaId,
			latestGroupChatMessageStanzaTimestamp, readMarkerPending, pinningPosition,
			chatStateSendingEnabled, readMarkerSendingEnabled, 0,
			automaticMediaDownloadsRule
			FROM roster
			WHERE NOT notificationsMuted
		)"));

    execQuery(query, QStringLiteral("DROP TABLE roster"));
    execQuery(query,
              SQL_CREATE_TABLE("roster",
                               SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(name, SQL_TEXT)
                                   SQL_ATTRIBUTE(subscription, SQL_INTEGER) SQL_ATTRIBUTE(groupChatParticipantId, SQL_TEXT)
                                       SQL_ATTRIBUTE(groupChatName, SQL_TEXT) SQL_ATTRIBUTE(groupChatDescription, SQL_TEXT)
                                           SQL_ATTRIBUTE(groupChatFlags, SQL_INTEGER) SQL_ATTRIBUTE(encryption, SQL_INTEGER)
                                               SQL_ATTRIBUTE(unreadMessages, SQL_INTEGER) SQL_ATTRIBUTE(lastReadOwnMessageId, SQL_TEXT)
                                                   SQL_ATTRIBUTE(lastReadContactMessageId, SQL_TEXT) SQL_ATTRIBUTE(latestGroupChatMessageStanzaId, SQL_TEXT)
                                                       SQL_ATTRIBUTE(latestGroupChatMessageStanzaTimestamp, SQL_TEXT) SQL_ATTRIBUTE(readMarkerPending, SQL_BOOL)
                                                           SQL_ATTRIBUTE(pinningPosition, SQL_INTEGER_NOT_NULL) SQL_ATTRIBUTE(chatStateSendingEnabled, SQL_BOOL)
                                                               SQL_ATTRIBUTE(readMarkerSendingEnabled, SQL_BOOL) SQL_ATTRIBUTE(notificationRule, SQL_INTEGER)
                                                                   SQL_ATTRIBUTE(automaticMediaDownloadsRule, SQL_INTEGER) "PRIMARY KEY(accountJid, jid)"));

    execQuery(query, QStringLiteral("INSERT INTO roster SELECT * FROM roster_tmp"));
    execQuery(query, QStringLiteral("DROP TABLE roster_tmp"));

    d->version = 44;
}

void Database::convertDatabaseToV45()
{
    DATABASE_CONVERT_TO_VERSION(44)
    QSqlQuery query(currentDatabase());

    // Add the columns "replyTo", "replyId" and "replyQuote".
    execQuery(
        query,
        SQL_CREATE_TABLE(
            "messages_tmp",
            SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(chatJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(isOwn, SQL_BOOL)
                SQL_ATTRIBUTE(groupChatSenderId, SQL_TEXT) SQL_ATTRIBUTE(id, SQL_TEXT) SQL_ATTRIBUTE(originId, SQL_TEXT) SQL_ATTRIBUTE(stanzaId, SQL_TEXT)
                    SQL_ATTRIBUTE(replaceId, SQL_TEXT) SQL_ATTRIBUTE(replyTo, SQL_TEXT) SQL_ATTRIBUTE(replyId, SQL_TEXT) SQL_ATTRIBUTE(replyQuote, SQL_TEXT)
                        SQL_ATTRIBUTE(timestamp, SQL_TEXT) SQL_ATTRIBUTE(body, SQL_TEXT) SQL_ATTRIBUTE(encryption, SQL_INTEGER)
                            SQL_ATTRIBUTE(senderKey, SQL_BLOB) SQL_ATTRIBUTE(deliveryState, SQL_INTEGER) SQL_ATTRIBUTE(isSpoiler, SQL_BOOL)
                                SQL_ATTRIBUTE(spoilerHint, SQL_TEXT) SQL_ATTRIBUTE(fileGroupId, SQL_INTEGER) SQL_ATTRIBUTE(groupChatInviterJid, SQL_TEXT)
                                    SQL_ATTRIBUTE(groupChatInviteeJid, SQL_TEXT) SQL_ATTRIBUTE(groupChatInvitationJid, SQL_TEXT)
                                        SQL_ATTRIBUTE(groupChatToken, SQL_TEXT) SQL_ATTRIBUTE(errorText, SQL_TEXT)
                                            SQL_ATTRIBUTE(removed, SQL_BOOL_NOT_NULL) "FOREIGN KEY(accountJid, chatJid) REFERENCES roster (accountJid, jid)"));

    execQuery(query, QStringLiteral(R"(
			INSERT INTO messages_tmp
			SELECT accountJid, chatJid, isOwn, groupChatSenderId, id, originId, stanzaId, replaceId,
			NULL, NULL, NULL, timestamp, body, encryption, senderKey, deliveryState, isSpoiler,
			spoilerHint, fileGroupId, groupChatInviterJid, groupChatInviteeJid,
			groupChatInvitationJid, groupChatToken, errorText, removed
			FROM messages
		)"));

    execQuery(query, QStringLiteral("DROP TABLE messages"));
    execQuery(
        query,
        SQL_CREATE_TABLE(
            "messages",
            SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(chatJid, SQL_TEXT_NOT_NULL) SQL_ATTRIBUTE(isOwn, SQL_BOOL_NOT_NULL)
                SQL_ATTRIBUTE(groupChatSenderId, SQL_TEXT) SQL_ATTRIBUTE(id, SQL_TEXT) SQL_ATTRIBUTE(originId, SQL_TEXT) SQL_ATTRIBUTE(stanzaId, SQL_TEXT)
                    SQL_ATTRIBUTE(replaceId, SQL_TEXT) SQL_ATTRIBUTE(replyTo, SQL_TEXT) SQL_ATTRIBUTE(replyId, SQL_TEXT) SQL_ATTRIBUTE(replyQuote, SQL_TEXT)
                        SQL_ATTRIBUTE(timestamp, SQL_TEXT) SQL_ATTRIBUTE(body, SQL_TEXT) SQL_ATTRIBUTE(encryption, SQL_INTEGER)
                            SQL_ATTRIBUTE(senderKey, SQL_BLOB) SQL_ATTRIBUTE(deliveryState, SQL_INTEGER) SQL_ATTRIBUTE(isSpoiler, SQL_BOOL)
                                SQL_ATTRIBUTE(spoilerHint, SQL_TEXT) SQL_ATTRIBUTE(fileGroupId, SQL_INTEGER) SQL_ATTRIBUTE(groupChatInviterJid, SQL_TEXT)
                                    SQL_ATTRIBUTE(groupChatInviteeJid, SQL_TEXT) SQL_ATTRIBUTE(groupChatInvitationJid, SQL_TEXT)
                                        SQL_ATTRIBUTE(groupChatToken, SQL_TEXT) SQL_ATTRIBUTE(errorText, SQL_TEXT)
                                            SQL_ATTRIBUTE(removed, SQL_BOOL_NOT_NULL) "FOREIGN KEY(accountJid, chatJid) REFERENCES roster (accountJid, jid)"));

    execQuery(query, QStringLiteral("INSERT INTO messages SELECT * FROM messages_tmp"));
    execQuery(query, QStringLiteral("DROP TABLE messages_tmp"));

    d->version = 45;
}

void Database::convertDatabaseToV46()
{
    DATABASE_CONVERT_TO_VERSION(45)
    QSqlQuery query(currentDatabase());
    execQuery(query, QStringLiteral("ALTER TABLE accounts ADD geoLocationMapPreviewEnabled " SQL_BOOL));
    execQuery(query, QStringLiteral("ALTER TABLE accounts ADD geoLocationMapService " SQL_INTEGER));
    d->version = 46;
}

void Database::convertDatabaseToV47()
{
    DATABASE_CONVERT_TO_VERSION(46)
    QSqlQuery query(currentDatabase());

    // Delete all rows used for file sharing.
    // That is needed because new files are stored with incremented IDs.
    // It allows retrieving files in the order they are sent/received.
    // Old files can have high IDs because they were randomly generated formerly.
    // Keeping them could result in a high ID used as the start for the new incremented IDs.
    execQuery(query, QStringLiteral("DELETE FROM messages WHERE fileGroupId IS NOT NULL"));
    execQuery(query, QStringLiteral("DELETE FROM files"));
    execQuery(query, QStringLiteral("DELETE FROM fileHashes"));
    execQuery(query, QStringLiteral("DELETE FROM fileHttpSources"));
    execQuery(query, QStringLiteral("DELETE FROM fileEncryptedSources"));

    d->version = 47;
}

void Database::convertDatabaseToV48()
{
    DATABASE_CONVERT_TO_VERSION(47)
    QSqlQuery query(currentDatabase());

    for (const auto &column : {
             SQL_LAST_ATTRIBUTE(online, SQL_BOOL),
             SQL_LAST_ATTRIBUTE(resourcePrefix, SQL_TEXT),
             SQL_LAST_ATTRIBUTE(password, SQL_TEXT),
             SQL_LAST_ATTRIBUTE(credentials, SQL_TEXT),
             SQL_LAST_ATTRIBUTE(host, SQL_TEXT),
             SQL_LAST_ATTRIBUTE(port, SQL_BOOL),
             SQL_LAST_ATTRIBUTE(tlsErrorsIgnored, SQL_INTEGER),
             SQL_LAST_ATTRIBUTE(tlsRequirement, SQL_INTEGER),
             SQL_LAST_ATTRIBUTE(passwordVisibility, SQL_INTEGER),
             SQL_LAST_ATTRIBUTE(userAgentDeviceId, SQL_TEXT),
             SQL_LAST_ATTRIBUTE(encryption, SQL_INTEGER),
             SQL_LAST_ATTRIBUTE(automaticMediaDownloadsRule, SQL_INTEGER),
         }) {
        execQuery(query, QStringLiteral("ALTER TABLE accounts ADD %1").arg(QString::fromUtf8(column)));
    }

    // Import account settings from settings file
    {
        const QString Online = QStringLiteral("auth/online");
        const QString Jid = QStringLiteral("auth/jid");
        const QString JidResourcePrefix = QStringLiteral("auth/jidResourcePrefix");
        const QString Password = QStringLiteral("auth/password");
        const QString Credentials = QStringLiteral("auth/credentials");
        const QString Host = QStringLiteral("auth/host");
        const QString Port = QStringLiteral("auth/port");
        const QString TlsErrorsIgnored = QStringLiteral("auth/tlsErrorsIgnored");
        const QString TlsRequirement = QStringLiteral("auth/tlsRequirement");
        const QString PasswordVisibility = QStringLiteral("auth/passwordVisibility");
        const QString UserAgentDeviceId = QStringLiteral("auth/userAgentDeviceId");
        const QString Encryption = QStringLiteral("encryption");
        const QString AutomaticDownloadsRule = QStringLiteral("media/automaticDownloadsRule");

        auto *settings = Settings::instance();
        const auto settingsAccount = [&]() -> std::optional<AccountSettings::Data> {
            const auto jid = settings->value<QString>(Jid);

            if (jid.isEmpty()) {
                return {};
            }

            AccountSettings::Data account;

            account.jid = jid;
            account.enabled = settings->value<bool>(Online, true);
            account.jidResourcePrefix = settings->value<QString>(JidResourcePrefix, QStringLiteral(KAIDAN_JID_RESOURCE_DEFAULT_PREFIX));
            account.password = QString::fromUtf8(QByteArray::fromBase64(settings->value<QString>(Password).toUtf8()));
            account.credentials = [&]() {
                QXmlStreamReader r(settings->value<QString>(Credentials));
                r.readNextStartElement();
                return QXmppCredentials::fromXml(r).value_or(QXmppCredentials());
            }();
            account.host = settings->value<QString>(Host);
            account.port = settings->value<quint16>(Port, AUTO_DETECT_PORT);
            account.tlsErrorsIgnored = settings->value<bool>(TlsErrorsIgnored, false);
            account.tlsRequirement = settings->value<QXmppConfiguration::StreamSecurityMode>(TlsRequirement, QXmppConfiguration::TLSRequired);
            account.passwordVisibility = settings->value<AccountSettings::PasswordVisibility>(PasswordVisibility, AccountSettings::PasswordVisibility::Visible);
            account.userAgentDeviceId = settings->value<QUuid>(UserAgentDeviceId);
            account.encryption = settings->value<Encryption::Enum>(Encryption, Encryption::Omemo2);
            account.automaticMediaDownloadsRule =
                settings->value<AccountSettings::AutomaticMediaDownloadsRule>(AutomaticDownloadsRule,
                                                                              AccountSettings::AutomaticMediaDownloadsRule::PresenceOnly);

            return account;
        }();

        if (settingsAccount) {
            const auto sqlAccount = [&]() -> AccountSettings::Data {
                execQuery(query, QStringLiteral("SELECT * FROM accounts"));
                QList<AccountSettings::Data> accounts;
                AccountDb::parseAccountsFromQuery(query, accounts);

                if (const auto it = std::find_if(accounts.cbegin(),
                                                 accounts.cend(),
                                                 [jid = settingsAccount->jid](const AccountSettings::Data &acc) {
                                                     return jid == acc.jid;
                                                 });
                    it != accounts.cend()) {
                    auto acc = *it;

                    acc.enabled = settingsAccount->enabled;
                    acc.jidResourcePrefix = settingsAccount->jidResourcePrefix;
                    acc.password = settingsAccount->password;
                    acc.credentials = settingsAccount->credentials;
                    acc.host = settingsAccount->host;
                    acc.port = settingsAccount->port;
                    acc.tlsErrorsIgnored = settingsAccount->tlsErrorsIgnored;
                    acc.tlsRequirement = settingsAccount->tlsRequirement;
                    acc.passwordVisibility = settingsAccount->passwordVisibility;
                    acc.userAgentDeviceId = settingsAccount->userAgentDeviceId;
                    acc.encryption = settingsAccount->encryption;
                    acc.automaticMediaDownloadsRule = settingsAccount->automaticMediaDownloadsRule;
                }

                return *settingsAccount;
            }();
            const auto record = AccountDb::createUpdateRecord({}, sqlAccount);
            const auto updateAccountSQL = [&]() -> QString {
                const auto driver = currentDatabase().driver();
                return QString(driver->sqlStatement(QSqlDriver::InsertStatement, QStringLiteral(DB_TABLE_ACCOUNTS), record, false))
                    .replace(QStringLiteral("INSERT "), QStringLiteral("INSERT OR REPLACE "));
            }();

            execQuery(query, updateAccountSQL);

            settings->remove({
                Online,
                Jid,
                JidResourcePrefix,
                Password,
                Credentials,
                Host,
                Port,
                TlsErrorsIgnored,
                TlsRequirement,
                PasswordVisibility,
                UserAgentDeviceId,
                Encryption,
                AutomaticDownloadsRule,
            });
        }
    }

    d->version = 48;
}

void Database::convertDatabaseToV49()
{
    DATABASE_CONVERT_TO_VERSION(48)
    QSqlQuery query(currentDatabase());
    execQuery(query, QStringLiteral("ALTER TABLE accounts RENAME COLUMN online TO enabled"));
    d->version = 49;
}

void Database::convertDatabaseToV50()
{
    DATABASE_CONVERT_TO_VERSION(49)
    QSqlQuery query(currentDatabase());
    execQuery(query, QStringLiteral("SELECT * FROM accounts"));
    QList<AccountSettings::Data> accounts;
    AccountDb::parseAccountsFromQuery(query, accounts);

    for (auto i = accounts.size() - 1; i >= 0; --i) {
        auto &account = accounts[i];

        if (auto decodedPassword = QByteArray::fromBase64Encoding(account.password.toUtf8())) {
            execQuery(query, QStringLiteral("UPDATE accounts SET password = '%1' WHERE jid = '%2'").arg(QString::fromUtf8(*decodedPassword), account.jid));
        }
    }

    d->version = 50;
}

#include "moc_Database.cpp"
