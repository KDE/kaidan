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

#include "Globals.h"
#include "Kaidan.h"
#include "SqlUtils.h"

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
#include <QThreadStorage>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrentRun>

#ifdef DB_UNIT_TEST
#define TEST_DB_FILENAME "tests_db_" + QCoreApplication::applicationName() + ".sqlite"
#include <QFile>
#include <QCoreApplication>
#endif

using namespace SqlUtils;

#define DATABASE_CONVERT_TO_VERSION(n) \
	if (d->version < n) { \
		convertDatabaseToV##n(); \
	}

// Both need to be updated on version bump:
#define DATABASE_LATEST_VERSION 40
#define DATABASE_CONVERT_TO_LATEST_VERSION() DATABASE_CONVERT_TO_VERSION(40)

#define SQL_BOOL "BOOL"
#define SQL_BOOL_NOT_NULL "BOOL NOT NULL"
#define SQL_INTEGER "INTEGER"
#define SQL_INTEGER_NOT_NULL "INTEGER NOT NULL"
#define SQL_TEXT "TEXT"
#define SQL_TEXT_NOT_NULL "TEXT NOT NULL"
#define SQL_BLOB "BLOB"
#define SQL_BLOB_NOT_NULL "BLOB NOT NULL"

#define SQL_CREATE_TABLE(tableName, contents) \
	"CREATE TABLE '" tableName "' (" contents ")"

#define SQL_LAST_ATTRIBUTE(name, dataType) \
	"'" QT_STRINGIFY(name) "' " dataType

#define SQL_ATTRIBUTE(name, dataType) \
	SQL_LAST_ATTRIBUTE(name, dataType) ","

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

#ifdef DB_UNIT_TEST
		const QString databaseFilePath = TEST_DB_FILENAME;
#else
		// Check if there is a writable location for app data.
		const auto appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
		if (appDataPath.isEmpty()) {
			qFatal("Failed to find writable location for database file.");
		}

		const auto appDataDir = QDir(appDataPath);
		if (!appDataDir.mkpath(QLatin1String("."))) {
			qFatal("Failed to create writable directory at %s",
				qPrintable(appDataDir.absolutePath()));
		}

		// Create the absolute database file path while ensuring that there is a writable
		// location on all systems.
		const auto databaseFilePath = appDataDir.absoluteFilePath(databaseFilename());

		// Rename old database files to the current name.
		// There should only be one old file at once.
		// Thus, only the first entry of "oldDbFilenames" is renamed.
		if (const auto oldDbFilenames = appDataDir.entryList(oldDatabaseFilenames(), QDir::Files);
			!oldDbFilenames.isEmpty()) {
			QFile::rename(appDataDir.absoluteFilePath(oldDbFilenames.first()), databaseFilePath);
		}
#endif
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
	DbNotCreated = 0,  // no tables
	DbOldVersion = 1,  // Kaidan v0.3 or before
};

struct DatabasePrivate
{
	QThread dbThread;
	QObject *dbWorker = new QObject();
	QMutex tableCreationMutex;
	int version = DbNotLoaded;
	int transactions = 0;
	bool tablesCreated = false;
};

Database::Database(QObject *parent)
	: QObject(parent),
	  d(new DatabasePrivate)
{
	// start thread and set thread name
	d->dbThread.setObjectName(QStringLiteral("DB (SQLite)"));
	d->dbThread.start();
	// worker
	d->dbWorker->moveToThread(&d->dbThread);
	connect(&d->dbThread, &QThread::finished, d->dbWorker, &QObject::deleteLater);

#ifdef DB_UNIT_TEST
	QFile file(TEST_DB_FILENAME);
	if (file.exists()) {
		file.remove();
	}
#endif
}

Database::~Database()
{
	// wait for finished
	d->dbThread.quit();
	d->dbThread.wait();
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
			qWarning() << "Could not begin transaction on database:"
			           << db.lastError().text();
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
		qWarning() << "[Database] commit() called, but there's no transaction to commit.";
	}

	if (!transactions) {
		// no transaction requested anymore
		auto db = currentDatabase();
		if (!db.commit()) {
			qWarning() << "Could not commit transaction on database:"
			           << db.lastError().text();
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
	if (!tables.contains(DB_TABLE_INFO)) {
		if (tables.contains("Messages") &&
			tables.contains("Roster")) {
			// old Kaidan v0.1/v0.2 table
			d->version = DbOldVersion;
		} else {
			d->version = DbNotCreated;
		}
		// we've got all we want; do not query for a db version
		return;
	}

	QSqlQuery query(db);
	execQuery(query, "SELECT version FROM " DB_TABLE_INFO);

	QSqlRecord record = query.record();
	int versionCol = record.indexOf("version");

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
	updateRecord.append(createSqlField("version", d->version));

	auto db = currentDatabase();
	QSqlQuery query(db);
	execQuery(
		query,
		db.driver()->sqlStatement(
			QSqlDriver::UpdateStatement,
			DB_TABLE_INFO,
			updateRecord,
			false
		)
	);
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
		qDebug() << "[Database] Creating new database with latest version" << DATABASE_LATEST_VERSION;
		createNewDatabase();
	} else {
		qDebug() << "[Database] Converting database from version" << d->version << "to latest version" << DATABASE_LATEST_VERSION;
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
	execQuery(
		query,
		SQL_CREATE_TABLE(
			DB_TABLE_INFO,
			SQL_LAST_ATTRIBUTE(version, SQL_INTEGER_NOT_NULL)
		)
	);

	QSqlRecord insertRecord;
	insertRecord.append(createSqlField("version", DATABASE_LATEST_VERSION));
	execQuery(
		query,
		db.driver()->sqlStatement(
			QSqlDriver::InsertStatement,
			DB_TABLE_INFO,
			insertRecord,
			false
		)
	);

	// accounts
	execQuery(
		query,
		SQL_CREATE_TABLE(
			DB_TABLE_ACCOUNTS,
			SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(name, SQL_TEXT)
			SQL_ATTRIBUTE(latestMessageStanzaId, SQL_TEXT)
			SQL_ATTRIBUTE(latestMessageTimestamp, SQL_TEXT)
			"PRIMARY KEY(jid)"
		)
	);

	// roster
	execQuery(
		query,
		SQL_CREATE_TABLE(
			DB_TABLE_ROSTER,
			SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(name, SQL_TEXT)
			SQL_ATTRIBUTE(subscription, SQL_INTEGER)
			SQL_ATTRIBUTE(encryption, SQL_INTEGER)
			SQL_ATTRIBUTE(unreadMessages, SQL_INTEGER)
			SQL_ATTRIBUTE(lastReadOwnMessageId, SQL_TEXT)
			SQL_ATTRIBUTE(lastReadContactMessageId, SQL_TEXT)
			SQL_ATTRIBUTE(readMarkerPending, SQL_BOOL)
			SQL_ATTRIBUTE(pinningPosition, SQL_INTEGER_NOT_NULL)
			SQL_ATTRIBUTE(chatStateSendingEnabled, SQL_BOOL)
			SQL_ATTRIBUTE(readMarkerSendingEnabled, SQL_BOOL)
			SQL_ATTRIBUTE(notificationsMuted, SQL_BOOL)
			SQL_ATTRIBUTE(automaticMediaDownloadsRule, SQL_INTEGER)
			"PRIMARY KEY(accountJid, jid)"
		)
	);
	execQuery(
		query,
		SQL_CREATE_TABLE(
			DB_TABLE_ROSTER_GROUPS,
			SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(chatJid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(name, SQL_TEXT_NOT_NULL)
			"PRIMARY KEY(accountJid, chatJid, name),"
			"FOREIGN KEY(accountJid) REFERENCES " DB_TABLE_ROSTER " (accountJid),"
			"FOREIGN KEY(chatJid) REFERENCES " DB_TABLE_ROSTER " (jid)"
		)
	);

	// messages
	execQuery(
		query,
		SQL_CREATE_TABLE(
			DB_TABLE_MESSAGES,
			SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(chatJid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(senderId, SQL_TEXT)
			SQL_ATTRIBUTE(id, SQL_TEXT)
			SQL_ATTRIBUTE(originId, SQL_TEXT)
			SQL_ATTRIBUTE(stanzaId, SQL_TEXT)
			SQL_ATTRIBUTE(replaceId, SQL_TEXT)
			SQL_ATTRIBUTE(timestamp, SQL_TEXT)
			SQL_ATTRIBUTE(body, SQL_TEXT)
			SQL_ATTRIBUTE(encryption, SQL_INTEGER)
			SQL_ATTRIBUTE(senderKey, SQL_BLOB)
			SQL_ATTRIBUTE(deliveryState, SQL_INTEGER)
			SQL_ATTRIBUTE(isSpoiler, SQL_BOOL)
			SQL_ATTRIBUTE(spoilerHint, SQL_TEXT)
			SQL_ATTRIBUTE(fileGroupId, SQL_INTEGER)
			SQL_ATTRIBUTE(errorText, SQL_TEXT)
			SQL_ATTRIBUTE(removed, SQL_BOOL_NOT_NULL)
			"FOREIGN KEY(accountJid, chatJid) REFERENCES roster (accountJid, jid)"
		)
	);
	execQuery(
		query,
		SQL_CREATE_TABLE(
			DB_TABLE_MESSAGE_REACTIONS,
			SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(chatJid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(messageSenderId, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(messageId, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(senderJid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(emoji, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(timestamp, SQL_INTEGER)
			SQL_ATTRIBUTE(deliveryState, SQL_INTEGER)
			"PRIMARY KEY(accountJid, chatJid, messageId, senderJid, emoji)"
		)
	);

	// file sharing
	execQuery(
		query,
		SQL_CREATE_TABLE(
			DB_TABLE_FILES,
			SQL_ATTRIBUTE(id, SQL_INTEGER_NOT_NULL)
			SQL_ATTRIBUTE(fileGroupId, SQL_INTEGER_NOT_NULL)
			SQL_ATTRIBUTE(name, SQL_TEXT)
			SQL_ATTRIBUTE(description, SQL_TEXT)
			SQL_ATTRIBUTE(mimeType, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(size, SQL_INTEGER)
			SQL_ATTRIBUTE(lastModified, SQL_INTEGER_NOT_NULL)
			SQL_ATTRIBUTE(disposition, SQL_INTEGER_NOT_NULL)
			SQL_ATTRIBUTE(thumbnail, SQL_BLOB)
			SQL_ATTRIBUTE(localFilePath, SQL_TEXT)
			"PRIMARY KEY(id)"
		)
	);
	execQuery(
		query,
		SQL_CREATE_TABLE(
			DB_TABLE_FILE_HASHES,
			SQL_ATTRIBUTE(dataId, SQL_INTEGER_NOT_NULL)
			SQL_ATTRIBUTE(hashType, SQL_INTEGER_NOT_NULL)
			SQL_ATTRIBUTE(hashValue, SQL_BLOB_NOT_NULL)
			"PRIMARY KEY(dataId, hashType)"
		)
	);
	execQuery(
		query,
		SQL_CREATE_TABLE(
			DB_TABLE_FILE_HTTP_SOURCES,
			SQL_ATTRIBUTE(fileId, SQL_INTEGER_NOT_NULL)
			SQL_ATTRIBUTE(url, SQL_BLOB_NOT_NULL)
			"PRIMARY KEY(fileId)"
		)
	);
	execQuery(
		query,
		SQL_CREATE_TABLE(
			DB_TABLE_FILE_ENCRYPTED_SOURCES,
			SQL_ATTRIBUTE(fileId, SQL_INTEGER_NOT_NULL)
			SQL_ATTRIBUTE(url, SQL_BLOB_NOT_NULL)
			SQL_ATTRIBUTE(cipher, SQL_INTEGER_NOT_NULL)
			SQL_ATTRIBUTE(key, SQL_BLOB_NOT_NULL)
			SQL_ATTRIBUTE(iv, SQL_BLOB_NOT_NULL)
			SQL_ATTRIBUTE(encryptedDataId, SQL_INTEGER)
			"PRIMARY KEY(fileId)"
		)
	);

	// trust management
	execQuery(
		query,
		SQL_CREATE_TABLE(
			DB_TABLE_TRUST_SECURITY_POLICIES,
			SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(encryption, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(securityPolicy, SQL_INTEGER_NOT_NULL)
			"PRIMARY KEY(account, encryption)"
		)
	);
	execQuery(
		query,
		SQL_CREATE_TABLE(
			DB_TABLE_TRUST_OWN_KEYS,
			SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(encryption, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(keyId, SQL_BLOB_NOT_NULL)
			"PRIMARY KEY(account, encryption)"
		)
	);
	execQuery(
		query,
		SQL_CREATE_TABLE(
			DB_TABLE_TRUST_KEYS,
			SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(encryption, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(ownerJid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(keyId, SQL_BLOB_NOT_NULL)
			SQL_ATTRIBUTE(trustLevel, SQL_INTEGER_NOT_NULL)
			"PRIMARY KEY(account, encryption, keyId, ownerJid)"
		)
	);
	execQuery(
		query,
		SQL_CREATE_TABLE(
			DB_TABLE_TRUST_KEYS_UNPROCESSED,
			SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(encryption, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(senderKeyId, SQL_BLOB_NOT_NULL)
			SQL_ATTRIBUTE(ownerJid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(keyId, SQL_BLOB_NOT_NULL)
			SQL_ATTRIBUTE(trust, SQL_BOOL_NOT_NULL)
			"PRIMARY KEY(account, encryption, keyId, ownerJid)"
		)
	);

	// OMEMO data
	execQuery(
		query,
		SQL_CREATE_TABLE(
			DB_TABLE_OMEMO_OWN_DEVICES,
			SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(id, SQL_INTEGER_NOT_NULL)
			SQL_ATTRIBUTE(label, SQL_TEXT)
			SQL_ATTRIBUTE(privateKey, SQL_BLOB)
			SQL_ATTRIBUTE(publicKey, SQL_BLOB)
			SQL_ATTRIBUTE(latestSignedPreKeyId, SQL_INTEGER_NOT_NULL)
			SQL_ATTRIBUTE(latestPreKeyId, SQL_INTEGER_NOT_NULL)
			"PRIMARY KEY(account)"
		)
	);
	execQuery(
		query,
		SQL_CREATE_TABLE(
			DB_TABLE_OMEMO_DEVICES,
			SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(userJid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(id, SQL_INTEGER_NOT_NULL)
			SQL_ATTRIBUTE(label, SQL_TEXT)
			SQL_ATTRIBUTE(keyId, SQL_BLOB)
			SQL_ATTRIBUTE(session, SQL_BLOB)
			SQL_ATTRIBUTE(unrespondedStanzasSent, SQL_INTEGER " DEFAULT 0")
			SQL_ATTRIBUTE(unrespondedStanzasReceived, SQL_INTEGER " DEFAULT 0")
			SQL_ATTRIBUTE(removalTimestamp, SQL_INTEGER)
			"PRIMARY KEY(account, userJid, id)"
		)
	);
	execQuery(
		query,
		SQL_CREATE_TABLE(
			DB_TABLE_OMEMO_PRE_KEY_PAIRS,
			SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(id, SQL_BLOB_NOT_NULL)
			SQL_ATTRIBUTE(data, SQL_BLOB_NOT_NULL)
			"PRIMARY KEY(account, id)"
		)
	);
	execQuery(
		query,
		SQL_CREATE_TABLE(
			DB_TABLE_OMEMO_SIGNED_PRE_KEY_PAIRS,
			SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(id, SQL_BLOB_NOT_NULL)
			SQL_ATTRIBUTE(data, SQL_BLOB_NOT_NULL)
			SQL_ATTRIBUTE(creationTimestamp, SQL_INTEGER)
			"PRIMARY KEY(account, id)"
		)
	);

	// blocked
	execQuery(
		query,
		SQL_CREATE_TABLE(
			DB_TABLE_BLOCKED,
			SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL)
			"PRIMARY KEY(accountJid, jid)"
		)
	);

	execQuery(query, "CREATE VIEW " DB_VIEW_CHAT_MESSAGES " AS SELECT * FROM " DB_TABLE_MESSAGES
					 " WHERE deliveryState != 4 AND removed != 1");
	execQuery(query, "CREATE VIEW " DB_VIEW_DRAFT_MESSAGES " AS SELECT * FROM " DB_TABLE_MESSAGES
					 " WHERE deliveryState = 4");

	d->version = DATABASE_LATEST_VERSION;
}

void Database::convertDatabaseToV2()
{
	// create a new dbinfo table
	QSqlQuery query(currentDatabase());
	execQuery(
		query,
		SQL_CREATE_TABLE(
			DB_TABLE_INFO,
			SQL_LAST_ATTRIBUTE(version, SQL_INTEGER_NOT_NULL)
		)
	);
	execQuery(query, "INSERT INTO dbinfo VALUES (:version)", {{ u":version", 2 }});
	d->version = 2;
}

void Database::convertDatabaseToV3()
{
	DATABASE_CONVERT_TO_VERSION(2);
	QSqlQuery query(currentDatabase());
	execQuery(query, "ALTER TABLE Roster ADD avatarHash " SQL_TEXT);
	d->version = 3;
}

void Database::convertDatabaseToV4()
{
	DATABASE_CONVERT_TO_VERSION(3);
	QSqlQuery query(currentDatabase());
	// SQLite doesn't support the ALTER TABLE drop columns feature, so we have to use a workaround.
	// we copy all rows into a back-up table (but without `avatarHash`), and then delete the old table
	// and copy everything to the normal table again
	execQuery(query, "CREATE TEMPORARY TABLE roster_backup(jid, name, lastExchanged,"
							"unreadMessages, lastMessage, lastOnline, activity, status, mood)");
	execQuery(query, "INSERT INTO roster_backup SELECT jid, name, lastExchanged, unreadMessages,"
							"lastMessage, lastOnline, activity, status, mood FROM " DB_TABLE_ROSTER);
	execQuery(query, "DROP TABLE Roster");
	execQuery(query, "CREATE TABLE Roster (jid " SQL_TEXT_NOT_NULL ", name " SQL_TEXT_NOT_NULL ","
							"lastExchanged " SQL_TEXT_NOT_NULL ", unreadMessages " SQL_INTEGER ", lastMessage  " SQL_TEXT ","
							"lastOnline " SQL_TEXT ", activity " SQL_TEXT ", status " SQL_TEXT ", mood " SQL_TEXT ")");
	execQuery(query, "INSERT INTO Roster SELECT jid, name, lastExchanged, unreadMessages,"
							"lastMessage, lastOnline, activity, status, mood FROM Roster_backup");
	execQuery(query, "DROP TABLE Roster_backup");
	d->version = 4;
}

void Database::convertDatabaseToV5()
{
	DATABASE_CONVERT_TO_VERSION(4);
	QSqlQuery query(currentDatabase());
	execQuery(query, "ALTER TABLE Messages ADD type " SQL_INTEGER);
	execQuery(query, "UPDATE Messages SET type = 0 WHERE type IS NULL");
	execQuery(query, "ALTER TABLE Messages ADD mediaUrl " SQL_TEXT);
	d->version = 5;
}

void Database::convertDatabaseToV6()
{
	DATABASE_CONVERT_TO_VERSION(5);
	QSqlQuery query(currentDatabase());
	const QStringView newColumns[] {
		u"mediaSize " SQL_INTEGER,
		u"mediaContentType " SQL_TEXT,
		u"mediaLastModified " SQL_INTEGER,
		u"mediaLocation " SQL_TEXT
	};
	for (auto column : newColumns) {
		execQuery(query, u"ALTER TABLE Messages ADD " % column);
	}
	d->version = 6;
}

void Database::convertDatabaseToV7()
{
	DATABASE_CONVERT_TO_VERSION(6);
	QSqlQuery query(currentDatabase());
	execQuery(query, "ALTER TABLE Messages ADD mediaThumb " SQL_BLOB);
	execQuery(query, "ALTER TABLE Messages ADD mediaHashes " SQL_TEXT);
	d->version = 7;
}

void Database::convertDatabaseToV8()
{
	DATABASE_CONVERT_TO_VERSION(7);
	QSqlQuery query(currentDatabase());
	execQuery(query, "CREATE TEMPORARY TABLE roster_backup(jid, name, lastExchanged, unreadMessages, lastMessage)");
	execQuery(query, "INSERT INTO roster_backup SELECT jid, name, lastExchanged, unreadMessages, lastMessage FROM Roster");
	execQuery(query, "DROP TABLE Roster");
	execQuery(
		query,
		SQL_CREATE_TABLE(
			"Roster",
			SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(name, SQL_TEXT)
			SQL_ATTRIBUTE(lastExchanged, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(unreadMessages, SQL_INTEGER)
			SQL_LAST_ATTRIBUTE(lastMessage, SQL_TEXT_NOT_NULL)
		)
	);

	execQuery(query, "INSERT INTO Roster SELECT jid, name, lastExchanged, unreadMessages, lastMessage FROM Roster_backup");
	execQuery(query, "DROP TABLE roster_backup");
	d->version = 8;
}

void Database::convertDatabaseToV9()
{
	DATABASE_CONVERT_TO_VERSION(8);
	QSqlQuery query(currentDatabase());
	execQuery(query, "ALTER TABLE Messages ADD edited " SQL_BOOL);
	d->version = 9;
}

void Database::convertDatabaseToV10()
{
	DATABASE_CONVERT_TO_VERSION(9);
	QSqlQuery query(currentDatabase());
	execQuery(query, "ALTER TABLE Messages ADD isSpoiler " SQL_BOOL);
	execQuery(query, "ALTER TABLE Messages ADD spoilerHint " SQL_TEXT);
	d->version = 10;
}

void Database::convertDatabaseToV11()
{
	DATABASE_CONVERT_TO_VERSION(10);
	QSqlQuery query(currentDatabase());
	execQuery(query, "ALTER TABLE Messages ADD deliveryState " SQL_INTEGER);
	execQuery(query, "UPDATE Messages SET deliveryState = 2 WHERE isDelivered = 1");
	execQuery(query, "UPDATE Messages SET deliveryState = 1 WHERE deliveryState IS NULL");
	execQuery(query, "ALTER TABLE Messages ADD errorText " SQL_TEXT);
	d->version = 11;
}

void Database::convertDatabaseToV12()
{
	DATABASE_CONVERT_TO_VERSION(11);
	QSqlQuery query(currentDatabase());
	execQuery(query, "ALTER TABLE Messages ADD replaceId " SQL_TEXT);
	d->version = 12;
}

void Database::convertDatabaseToV13()
{
	DATABASE_CONVERT_TO_VERSION(12);
	QSqlQuery query(currentDatabase());
	execQuery(query, "ALTER TABLE Messages ADD stanzaId " SQL_TEXT);
	execQuery(query, "ALTER TABLE Messages ADD originId " SQL_TEXT);
	d->version = 13;
}

void Database::convertDatabaseToV14()
{
	DATABASE_CONVERT_TO_VERSION(13);
	QSqlQuery query(currentDatabase());
	execQuery(query, "ALTER TABLE Roster ADD subscription " SQL_INTEGER);
	execQuery(query, "ALTER TABLE Roster ADD encryption " SQL_INTEGER);
	execQuery(query, "ALTER TABLE Messages ADD encryption " SQL_INTEGER);
	execQuery(query, "ALTER TABLE Messages ADD senderKey " SQL_BLOB);
	d->version = 14;
}

void Database::convertDatabaseToV15()
{
	DATABASE_CONVERT_TO_VERSION(14);
	QSqlQuery query(currentDatabase());
	execQuery(
		query,
		SQL_CREATE_TABLE(
			"trust_security_policies",
			SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(encryption, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(security_policy, SQL_INTEGER_NOT_NULL)
			"PRIMARY KEY(account, encryption)"
		)
	);
	execQuery(
		query,
		SQL_CREATE_TABLE(
			"trust_own_keys",
			SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(encryption, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(key_id, SQL_BLOB_NOT_NULL)
			"PRIMARY KEY(account, encryption)"
		)
	);
	execQuery(
		query,
		SQL_CREATE_TABLE(
			"trust_keys",
			SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(encryption, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(key_id, SQL_BLOB_NOT_NULL)
			SQL_ATTRIBUTE(owner_jid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(trust_level, SQL_INTEGER_NOT_NULL)
			"PRIMARY KEY(account, encryption, key_id, owner_jid)"
		)
	);
	execQuery(
		query,
		SQL_CREATE_TABLE(
			"trust_keys_unprocessed",
			SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(encryption, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(key_id, SQL_BLOB_NOT_NULL)
			SQL_ATTRIBUTE(owner_jid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(sender_key_id, SQL_BLOB_NOT_NULL)
			SQL_ATTRIBUTE(trust, SQL_BOOL_NOT_NULL)
			"PRIMARY KEY(account, encryption, key_id, owner_jid)"
		)
	);
	d->version = 15;
}

void Database::convertDatabaseToV16()
{
	DATABASE_CONVERT_TO_VERSION(15);
	QSqlQuery query(currentDatabase());

	execQuery(
		query,
		SQL_CREATE_TABLE(
			"omemo_devices_own",
			SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(id, SQL_INTEGER_NOT_NULL)
			SQL_ATTRIBUTE(label, SQL_TEXT)
			SQL_ATTRIBUTE(private_key, SQL_BLOB)
			SQL_ATTRIBUTE(public_key, SQL_BLOB)
			SQL_ATTRIBUTE(latest_signed_pre_key_id, SQL_INTEGER_NOT_NULL)
			SQL_ATTRIBUTE(latest_pre_key_id, SQL_INTEGER_NOT_NULL)
			"PRIMARY KEY(account)"
		)
	);
	execQuery(
		query,
		SQL_CREATE_TABLE(
			"omemo_devices",
			SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(user_jid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(id, SQL_INTEGER_NOT_NULL)
			SQL_ATTRIBUTE(label, SQL_TEXT)
			SQL_ATTRIBUTE(key_id, SQL_BLOB)
			SQL_ATTRIBUTE(session, SQL_BLOB)
			SQL_ATTRIBUTE(unresponded_stanzas_sent, SQL_INTEGER " DEFAULT 0")
			SQL_ATTRIBUTE(unresponded_stanzas_received, SQL_INTEGER " DEFAULT 0")
			SQL_ATTRIBUTE(removal_timestamp, SQL_INTEGER)
			"PRIMARY KEY(account, user_jid, id)"
		)
	);
	execQuery(
		query,
		SQL_CREATE_TABLE(
			"omemo_pre_key_pairs",
			SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(id, SQL_BLOB_NOT_NULL)
			SQL_ATTRIBUTE(data, SQL_BLOB_NOT_NULL)
			"PRIMARY KEY(id)"
		)
	);
	execQuery(
		query,
		SQL_CREATE_TABLE(
			"omemo_pre_key_pairs_signed",
			SQL_ATTRIBUTE(account, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(id, SQL_BLOB_NOT_NULL)
			SQL_ATTRIBUTE(data, SQL_BLOB_NOT_NULL)
			SQL_ATTRIBUTE(creation_timestamp, SQL_INTEGER)
			"PRIMARY KEY(id)"
		)
	);

	d->version = 16;
}

void Database::convertDatabaseToV17()
{
	DATABASE_CONVERT_TO_VERSION(16);
	QSqlQuery query(currentDatabase());
	execQuery(query, "ALTER TABLE Roster ADD lastReadOwnMessageId " SQL_TEXT);
	execQuery(query, "ALTER TABLE Roster ADD lastReadContactMessageId " SQL_TEXT);
	execQuery(query, "ALTER TABLE Messages ADD isMarkable " SQL_BOOL);
	d->version = 17;
}

void Database::convertDatabaseToV18()
{
	DATABASE_CONVERT_TO_VERSION(17)
	QSqlQuery query(currentDatabase());

	// manually convert messages table
	execQuery(
		query,
		SQL_CREATE_TABLE(
			"messages_tmp",
			SQL_ATTRIBUTE(sender, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(recipient, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(timestamp, SQL_TEXT)
			SQL_ATTRIBUTE(message, SQL_TEXT)
			SQL_ATTRIBUTE(id, SQL_TEXT)
			SQL_ATTRIBUTE(encryption, SQL_INTEGER)
			SQL_ATTRIBUTE(senderKey, SQL_BLOB)
			SQL_ATTRIBUTE(deliveryState, SQL_INTEGER)
			SQL_ATTRIBUTE(isMarkable, SQL_BOOL)
			SQL_ATTRIBUTE(mediaUrl, SQL_TEXT)
			SQL_ATTRIBUTE(mediaLocation, SQL_TEXT)
			SQL_ATTRIBUTE(isEdited, SQL_BOOL)
			SQL_ATTRIBUTE(spoilerHint, SQL_TEXT)
			SQL_ATTRIBUTE(isSpoiler, SQL_BOOL)
			SQL_ATTRIBUTE(errorText, SQL_TEXT)
			SQL_ATTRIBUTE(replaceId, SQL_TEXT)
			SQL_ATTRIBUTE(originId, SQL_TEXT)
			SQL_ATTRIBUTE(stanzaId, SQL_TEXT)
			SQL_ATTRIBUTE(file_group_id, SQL_INTEGER)
			"FOREIGN KEY(sender) REFERENCES Roster (jid),"
			"FOREIGN KEY(recipient) REFERENCES Roster (jid)"
		)
	);

	execQuery(
		query,
		"INSERT INTO messages_tmp SELECT author, recipient, timestamp, message, id, encryption, "
		"senderKey, deliveryState, isMarkable, mediaUrl, mediaLocation, edited, spoilerHint, "
		"isSpoiler, errorText, replaceId, originId, stanzaId, NULL FROM Messages"
	);

	execQuery(query, "DROP TABLE Messages");

	execQuery(
		query,
		SQL_CREATE_TABLE(
			"messages",
			SQL_ATTRIBUTE(sender, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(recipient, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(timestamp, SQL_TEXT)
			SQL_ATTRIBUTE(message, SQL_TEXT)
			SQL_ATTRIBUTE(id, SQL_TEXT)
			SQL_ATTRIBUTE(encryption, SQL_INTEGER)
			SQL_ATTRIBUTE(senderKey, SQL_BLOB)
			SQL_ATTRIBUTE(deliveryState, SQL_INTEGER)
			SQL_ATTRIBUTE(isMarkable, SQL_BOOL)
			SQL_ATTRIBUTE(mediaUrl, SQL_TEXT)
			SQL_ATTRIBUTE(mediaLocation, SQL_TEXT)
			SQL_ATTRIBUTE(isEdited, SQL_BOOL)
			SQL_ATTRIBUTE(spoilerHint, SQL_TEXT)
			SQL_ATTRIBUTE(isSpoiler, SQL_BOOL)
			SQL_ATTRIBUTE(errorText, SQL_TEXT)
			SQL_ATTRIBUTE(replaceId, SQL_TEXT)
			SQL_ATTRIBUTE(originId, SQL_TEXT)
			SQL_ATTRIBUTE(stanzaId, SQL_TEXT)
			SQL_ATTRIBUTE(file_group_id, SQL_INTEGER)
			"FOREIGN KEY(sender) REFERENCES Roster (jid),"
			"FOREIGN KEY(recipient) REFERENCES Roster (jid)"
		)
	);

	execQuery(query, "INSERT INTO messages SELECT * FROM messages_tmp");
	execQuery(query, "DROP TABLE messages_tmp");

	d->version = 18;
}

void Database::convertDatabaseToV19()
{
	DATABASE_CONVERT_TO_VERSION(18)
	QSqlQuery query(currentDatabase());

	// file sharing
	execQuery(
		query,
		SQL_CREATE_TABLE(
			"files",
			SQL_ATTRIBUTE(id, SQL_INTEGER_NOT_NULL)
			SQL_ATTRIBUTE(file_group_id, SQL_INTEGER_NOT_NULL)
			SQL_ATTRIBUTE(name, SQL_TEXT)
			SQL_ATTRIBUTE(description, SQL_TEXT)
			SQL_ATTRIBUTE(mime_type, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(size, SQL_INTEGER)
			SQL_ATTRIBUTE(last_modified, SQL_INTEGER_NOT_NULL)
			SQL_ATTRIBUTE(disposition, SQL_INTEGER_NOT_NULL)
			SQL_ATTRIBUTE(thumbnail, SQL_BLOB)
			SQL_ATTRIBUTE(local_file_path, SQL_TEXT)
			"PRIMARY KEY(id)"
		)
	);
	execQuery(
		query,
		SQL_CREATE_TABLE(
			"file_hashes",
			SQL_ATTRIBUTE(data_id, SQL_INTEGER_NOT_NULL)
			SQL_ATTRIBUTE(hash_type, SQL_INTEGER_NOT_NULL)
			SQL_ATTRIBUTE(hash_value, SQL_BLOB_NOT_NULL)
			"PRIMARY KEY(data_id, hash_type)"
		)
	);
	execQuery(
		query,
		SQL_CREATE_TABLE(
			"file_http_sources",
			SQL_ATTRIBUTE(file_id, SQL_INTEGER_NOT_NULL)
			SQL_ATTRIBUTE(url, SQL_BLOB_NOT_NULL)
			"PRIMARY KEY(file_id)"
		)
	);
	execQuery(
		query,
		SQL_CREATE_TABLE(
			"file_encrypted_sources",
			SQL_ATTRIBUTE(file_id, SQL_INTEGER_NOT_NULL)
			SQL_ATTRIBUTE(url, SQL_BLOB_NOT_NULL)
			SQL_ATTRIBUTE(cipher, SQL_INTEGER_NOT_NULL)
			SQL_ATTRIBUTE(key, SQL_BLOB_NOT_NULL)
			SQL_ATTRIBUTE(iv, SQL_BLOB_NOT_NULL)
			SQL_ATTRIBUTE(encrypted_data_id, SQL_INTEGER)
			"PRIMARY KEY(file_id)"
		)
	);

	// manually convert messages table
	execQuery(
		query,
		SQL_CREATE_TABLE(
			"messages_tmp",
			SQL_ATTRIBUTE(sender, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(recipient, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(timestamp, SQL_TEXT)
			SQL_ATTRIBUTE(message, SQL_TEXT)
			SQL_ATTRIBUTE(id, SQL_TEXT)
			SQL_ATTRIBUTE(encryption, SQL_INTEGER)
			SQL_ATTRIBUTE(senderKey, SQL_BLOB)
			SQL_ATTRIBUTE(deliveryState, SQL_INTEGER)
			SQL_ATTRIBUTE(isMarkable, SQL_BOOL)
			SQL_ATTRIBUTE(isEdited, SQL_BOOL)
			SQL_ATTRIBUTE(spoilerHint, SQL_TEXT)
			SQL_ATTRIBUTE(isSpoiler, SQL_BOOL)
			SQL_ATTRIBUTE(errorText, SQL_TEXT)
			SQL_ATTRIBUTE(replaceId, SQL_TEXT)
			SQL_ATTRIBUTE(originId, SQL_TEXT)
			SQL_ATTRIBUTE(stanzaId, SQL_TEXT)
			SQL_ATTRIBUTE(file_group_id, SQL_INTEGER)
			"FOREIGN KEY(sender) REFERENCES Roster (jid),"
			"FOREIGN KEY(recipient) REFERENCES Roster (jid)"
		)
	);

	execQuery(
		query,
		"INSERT INTO messages_tmp SELECT sender, recipient, timestamp, message, id, encryption, "
		"senderKey, deliveryState, isMarkable, isEdited, spoilerHint, isSpoiler, errorText, "
		"replaceId, originId, stanzaId, file_group_id FROM messages"
	);

	execQuery(query, "DROP TABLE messages");

	execQuery(
		query,
		SQL_CREATE_TABLE(
			"messages",
			SQL_ATTRIBUTE(sender, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(recipient, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(timestamp, SQL_TEXT)
			SQL_ATTRIBUTE(message, SQL_TEXT)
			SQL_ATTRIBUTE(id, SQL_TEXT)
			SQL_ATTRIBUTE(encryption, SQL_INTEGER)
			SQL_ATTRIBUTE(senderKey, SQL_BLOB)
			SQL_ATTRIBUTE(deliveryState, SQL_INTEGER)
			SQL_ATTRIBUTE(isMarkable, SQL_BOOL)
			SQL_ATTRIBUTE(isEdited, SQL_BOOL)
			SQL_ATTRIBUTE(spoilerHint, SQL_TEXT)
			SQL_ATTRIBUTE(isSpoiler, SQL_BOOL)
			SQL_ATTRIBUTE(errorText, SQL_TEXT)
			SQL_ATTRIBUTE(replaceId, SQL_TEXT)
			SQL_ATTRIBUTE(originId, SQL_TEXT)
			SQL_ATTRIBUTE(stanzaId, SQL_TEXT)
			SQL_ATTRIBUTE(file_group_id, SQL_INTEGER)
			"FOREIGN KEY(sender) REFERENCES Roster (jid),"
			"FOREIGN KEY(recipient) REFERENCES Roster (jid)"
		)
	);

	execQuery(query, "INSERT INTO messages SELECT * FROM messages_tmp");
	execQuery(query, "DROP TABLE messages_tmp");

	d->version = 19;
}

void Database::convertDatabaseToV20()
{
	DATABASE_CONVERT_TO_VERSION(19);
	QSqlQuery query(currentDatabase());
	execQuery(query, "ALTER TABLE Roster ADD readMarkerPending " SQL_BOOL);
	d->version = 20;
}

void Database::convertDatabaseToV21()
{
	DATABASE_CONVERT_TO_VERSION(20);
	QSqlQuery query(currentDatabase());
	execQuery(query, "ALTER TABLE Roster ADD pinningPosition " SQL_INTEGER);
	d->version = 21;
}

void Database::convertDatabaseToV22()
{
	DATABASE_CONVERT_TO_VERSION(21);
	QSqlQuery query(currentDatabase());

	execQuery(
		query,
		SQL_CREATE_TABLE(
			"messageReactions",
			SQL_ATTRIBUTE(messageSender, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(messageRecipient, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(messageId, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(senderJid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(timestamp, SQL_INTEGER)
			SQL_ATTRIBUTE(emoji, SQL_TEXT_NOT_NULL)
			"PRIMARY KEY(messageSender, messageRecipient, messageId, senderJid, timestamp, emoji)"
		)
	);

	d->version = 22;
}

void Database::convertDatabaseToV23()
{
	DATABASE_CONVERT_TO_VERSION(22)
	QSqlQuery query(currentDatabase());

	// Rename the table "Roster" to "roster".
	// Remove the unused columns "lastExchanged" and "lastMessage".
	execQuery(
		query,
		SQL_CREATE_TABLE(
			"roster_tmp",
			SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(name, SQL_TEXT)
			SQL_ATTRIBUTE(subscription, SQL_INTEGER)
			SQL_ATTRIBUTE(encryption, SQL_INTEGER)
			SQL_ATTRIBUTE(unreadMessages, SQL_INTEGER)
			SQL_ATTRIBUTE(lastReadOwnMessageId, SQL_TEXT)
			SQL_ATTRIBUTE(lastReadContactMessageId, SQL_TEXT)
			SQL_ATTRIBUTE(readMarkerPending, SQL_BOOL)
			SQL_LAST_ATTRIBUTE(pinningPosition, SQL_INTEGER)
		)
	);

	execQuery(
		query,
		"INSERT INTO roster_tmp SELECT jid, name, subscription, encryption, unreadMessages, "
		"lastReadOwnMessageId, lastReadContactMessageId, readMarkerPending, pinningPosition "
		" FROM Roster"
	);

	execQuery(query, "DROP TABLE Roster");

	execQuery(
		query,
		SQL_CREATE_TABLE(
			"roster",
			SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(name, SQL_TEXT)
			SQL_ATTRIBUTE(subscription, SQL_INTEGER)
			SQL_ATTRIBUTE(encryption, SQL_INTEGER)
			SQL_ATTRIBUTE(unreadMessages, SQL_INTEGER)
			SQL_ATTRIBUTE(lastReadOwnMessageId, SQL_TEXT)
			SQL_ATTRIBUTE(lastReadContactMessageId, SQL_TEXT)
			SQL_ATTRIBUTE(readMarkerPending, SQL_BOOL)
			SQL_LAST_ATTRIBUTE(pinningPosition, SQL_INTEGER)
		)
	);

	execQuery(query, "INSERT INTO roster SELECT * FROM roster_tmp");
	execQuery(query, "DROP TABLE roster_tmp");

	// Adapt the foreign keys of table "messages" to new table name "roster".
	execQuery(
		query,
		SQL_CREATE_TABLE(
			"messages_tmp",
			SQL_ATTRIBUTE(sender, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(recipient, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(timestamp, SQL_TEXT)
			SQL_ATTRIBUTE(message, SQL_TEXT)
			SQL_ATTRIBUTE(id, SQL_TEXT)
			SQL_ATTRIBUTE(encryption, SQL_INTEGER)
			SQL_ATTRIBUTE(senderKey, SQL_BLOB)
			SQL_ATTRIBUTE(deliveryState, SQL_INTEGER)
			SQL_ATTRIBUTE(isMarkable, SQL_BOOL)
			SQL_ATTRIBUTE(isEdited, SQL_BOOL)
			SQL_ATTRIBUTE(spoilerHint, SQL_TEXT)
			SQL_ATTRIBUTE(isSpoiler, SQL_BOOL)
			SQL_ATTRIBUTE(errorText, SQL_TEXT)
			SQL_ATTRIBUTE(replaceId, SQL_TEXT)
			SQL_ATTRIBUTE(originId, SQL_TEXT)
			SQL_ATTRIBUTE(stanzaId, SQL_TEXT)
			SQL_ATTRIBUTE(file_group_id, SQL_INTEGER)
			"FOREIGN KEY(sender) REFERENCES roster (jid),"
			"FOREIGN KEY(recipient) REFERENCES roster (jid)"
		)
	);

	execQuery(
		query,
		"INSERT INTO messages_tmp SELECT sender, recipient, timestamp, message, id, encryption, "
		"senderKey, deliveryState, isMarkable, isEdited, spoilerHint, isSpoiler, errorText, "
		"replaceId, originId, stanzaId, file_group_id FROM messages"
	);

	execQuery(query, "DROP TABLE messages");

	execQuery(
		query,
		SQL_CREATE_TABLE(
			"messages",
			SQL_ATTRIBUTE(sender, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(recipient, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(timestamp, SQL_TEXT)
			SQL_ATTRIBUTE(message, SQL_TEXT)
			SQL_ATTRIBUTE(id, SQL_TEXT)
			SQL_ATTRIBUTE(encryption, SQL_INTEGER)
			SQL_ATTRIBUTE(senderKey, SQL_BLOB)
			SQL_ATTRIBUTE(deliveryState, SQL_INTEGER)
			SQL_ATTRIBUTE(isMarkable, SQL_BOOL)
			SQL_ATTRIBUTE(isEdited, SQL_BOOL)
			SQL_ATTRIBUTE(spoilerHint, SQL_TEXT)
			SQL_ATTRIBUTE(isSpoiler, SQL_BOOL)
			SQL_ATTRIBUTE(errorText, SQL_TEXT)
			SQL_ATTRIBUTE(replaceId, SQL_TEXT)
			SQL_ATTRIBUTE(originId, SQL_TEXT)
			SQL_ATTRIBUTE(stanzaId, SQL_TEXT)
			SQL_ATTRIBUTE(file_group_id, SQL_INTEGER)
			"FOREIGN KEY(sender) REFERENCES roster (jid),"
			"FOREIGN KEY(recipient) REFERENCES roster (jid)"
		)
	);

	execQuery(query, "INSERT INTO messages SELECT * FROM messages_tmp");
	execQuery(query, "DROP TABLE messages_tmp");

	// Use camelCase for all tables.

	execQuery(query, "ALTER TABLE messages RENAME COLUMN file_group_id TO fileGroupId");

	execQuery(query, "ALTER TABLE files RENAME COLUMN file_group_id TO fileGroupId");
	execQuery(query, "ALTER TABLE files RENAME COLUMN mime_type TO mimeType");
	execQuery(query, "ALTER TABLE files RENAME COLUMN last_modified TO lastModified");
	execQuery(query, "ALTER TABLE files RENAME COLUMN local_file_path TO localFilePath");

	execQuery(query, "ALTER TABLE file_hashes RENAME TO fileHashes");
	execQuery(query, "ALTER TABLE fileHashes RENAME COLUMN data_id TO dataId");
	execQuery(query, "ALTER TABLE fileHashes RENAME COLUMN hash_type TO hashType");
	execQuery(query, "ALTER TABLE fileHashes RENAME COLUMN hash_value TO hashValue");

	execQuery(query, "ALTER TABLE file_http_sources RENAME TO fileHttpSources");
	execQuery(query, "ALTER TABLE fileHttpSources RENAME COLUMN file_id TO fileId");

	execQuery(query, "ALTER TABLE file_encrypted_sources RENAME TO fileEncryptedSources");
	execQuery(query, "ALTER TABLE fileEncryptedSources RENAME COLUMN file_id TO fileId");
	execQuery(query, "ALTER TABLE fileEncryptedSources RENAME COLUMN encrypted_data_id TO encryptedDataId");

	execQuery(query, "ALTER TABLE trust_security_policies RENAME TO trustSecurityPolicies");
	execQuery(query, "ALTER TABLE trustSecurityPolicies RENAME COLUMN security_policy TO securityPolicy");

	execQuery(query, "ALTER TABLE trust_own_keys RENAME TO trustOwnKeys");
	execQuery(query, "ALTER TABLE trustOwnKeys RENAME COLUMN key_id TO keyId");

	execQuery(query, "ALTER TABLE trust_keys RENAME TO trustKeys");
	execQuery(query, "ALTER TABLE trustKeys RENAME COLUMN owner_jid TO ownerJid");
	execQuery(query, "ALTER TABLE trustKeys RENAME COLUMN key_id TO keyId");
	execQuery(query, "ALTER TABLE trustKeys RENAME COLUMN trust_level TO trustLevel");

	execQuery(query, "ALTER TABLE trust_keys_unprocessed RENAME TO trustKeysUnprocessed");
	execQuery(query, "ALTER TABLE trustKeysUnprocessed RENAME COLUMN sender_key_id TO senderKeyId");
	execQuery(query, "ALTER TABLE trustKeysUnprocessed RENAME COLUMN owner_jid TO ownerJid");
	execQuery(query, "ALTER TABLE trustKeysUnprocessed RENAME COLUMN key_id TO keyId");

	execQuery(query, "ALTER TABLE omemo_devices_own RENAME TO omemoDevicesOwn");
	execQuery(query, "ALTER TABLE omemoDevicesOwn RENAME COLUMN private_key TO privateKey");
	execQuery(query, "ALTER TABLE omemoDevicesOwn RENAME COLUMN public_key TO publicKey");
	execQuery(query, "ALTER TABLE omemoDevicesOwn RENAME COLUMN latest_signed_pre_key_id TO latestSignedPreKeyId");
	execQuery(query, "ALTER TABLE omemoDevicesOwn RENAME COLUMN latest_pre_key_id TO latestPreKeyId");

	execQuery(query, "ALTER TABLE omemo_devices RENAME TO omemoDevices");
	execQuery(query, "ALTER TABLE omemoDevices RENAME COLUMN user_jid TO userJid");
	execQuery(query, "ALTER TABLE omemoDevices RENAME COLUMN key_id TO keyId");
	execQuery(query, "ALTER TABLE omemoDevices RENAME COLUMN unresponded_stanzas_sent TO unrespondedStanzasSent");
	execQuery(query, "ALTER TABLE omemoDevices RENAME COLUMN unresponded_stanzas_received TO unrespondedStanzasReceived");
	execQuery(query, "ALTER TABLE omemoDevices RENAME COLUMN removal_timestamp TO removalTimestamp");

	execQuery(query, "ALTER TABLE omemo_pre_key_pairs RENAME TO omemoPreKeyPairs");

	execQuery(query, "ALTER TABLE omemo_pre_key_pairs_signed RENAME TO omemoPreKeyPairsSigned");
	execQuery(query, "ALTER TABLE omemoPreKeyPairsSigned RENAME COLUMN creation_timestamp TO creationTimestamp");

	d->version = 23;
}

void Database::convertDatabaseToV24()
{
	DATABASE_CONVERT_TO_VERSION(23);
	QSqlQuery query(currentDatabase());
	execQuery(query, "ALTER TABLE Roster ADD chatStateSendingEnabled " SQL_BOOL);
	execQuery(query, "ALTER TABLE Roster ADD readMarkerSendingEnabled " SQL_BOOL);
	d->version = 24;
}

void Database::convertDatabaseToV25()
{
	DATABASE_CONVERT_TO_VERSION(24);
	QSqlQuery query(currentDatabase());

	// Remove the column "isMarkable".
	execQuery(
		query,
		SQL_CREATE_TABLE(
			"messages_tmp",
			SQL_ATTRIBUTE(sender, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(recipient, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(timestamp, SQL_TEXT)
			SQL_ATTRIBUTE(message, SQL_TEXT)
			SQL_ATTRIBUTE(id, SQL_TEXT)
			SQL_ATTRIBUTE(encryption, SQL_INTEGER)
			SQL_ATTRIBUTE(senderKey, SQL_BLOB)
			SQL_ATTRIBUTE(deliveryState, SQL_INTEGER)
			SQL_ATTRIBUTE(isEdited, SQL_BOOL)
			SQL_ATTRIBUTE(spoilerHint, SQL_TEXT)
			SQL_ATTRIBUTE(isSpoiler, SQL_BOOL)
			SQL_ATTRIBUTE(errorText, SQL_TEXT)
			SQL_ATTRIBUTE(replaceId, SQL_TEXT)
			SQL_ATTRIBUTE(originId, SQL_TEXT)
			SQL_ATTRIBUTE(stanzaId, SQL_TEXT)
			SQL_ATTRIBUTE(fileGroupId, SQL_INTEGER)
			"FOREIGN KEY(sender) REFERENCES roster (jid),"
			"FOREIGN KEY(recipient) REFERENCES roster (jid)"
		)
	);

	execQuery(
		query,
		"INSERT INTO messages_tmp SELECT sender, recipient, timestamp, message, id, encryption, "
		"senderKey, deliveryState, isEdited, spoilerHint, isSpoiler, errorText, replaceId, "
		"originId, stanzaId, fileGroupId FROM messages"
	);

	execQuery(query, "DROP TABLE messages");

	execQuery(
		query,
		SQL_CREATE_TABLE(
			"messages",
			SQL_ATTRIBUTE(sender, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(recipient, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(timestamp, SQL_TEXT)
			SQL_ATTRIBUTE(message, SQL_TEXT)
			SQL_ATTRIBUTE(id, SQL_TEXT)
			SQL_ATTRIBUTE(encryption, SQL_INTEGER)
			SQL_ATTRIBUTE(senderKey, SQL_BLOB)
			SQL_ATTRIBUTE(deliveryState, SQL_INTEGER)
			SQL_ATTRIBUTE(isEdited, SQL_BOOL)
			SQL_ATTRIBUTE(spoilerHint, SQL_TEXT)
			SQL_ATTRIBUTE(isSpoiler, SQL_BOOL)
			SQL_ATTRIBUTE(errorText, SQL_TEXT)
			SQL_ATTRIBUTE(replaceId, SQL_TEXT)
			SQL_ATTRIBUTE(originId, SQL_TEXT)
			SQL_ATTRIBUTE(stanzaId, SQL_TEXT)
			SQL_ATTRIBUTE(fileGroupId, SQL_INTEGER)
			"FOREIGN KEY(sender) REFERENCES roster (jid),"
			"FOREIGN KEY(recipient) REFERENCES roster (jid)"
		)
	);

	execQuery(query, "INSERT INTO messages SELECT * FROM messages_tmp");
	execQuery(query, "DROP TABLE messages_tmp");

	d->version = 25;
}

void Database::convertDatabaseToV26()
{
	DATABASE_CONVERT_TO_VERSION(25)
	QSqlQuery query(currentDatabase());

	// Make the column "pinningPosition" NOT NULL and set existing rows to -1.
	execQuery(
		query,
		SQL_CREATE_TABLE(
			"roster_tmp",
			SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(name, SQL_TEXT)
			SQL_ATTRIBUTE(subscription, SQL_INTEGER)
			SQL_ATTRIBUTE(encryption, SQL_INTEGER)
			SQL_ATTRIBUTE(unreadMessages, SQL_INTEGER)
			SQL_ATTRIBUTE(lastReadOwnMessageId, SQL_TEXT)
			SQL_ATTRIBUTE(lastReadContactMessageId, SQL_TEXT)
			SQL_ATTRIBUTE(readMarkerPending, SQL_BOOL)
			SQL_ATTRIBUTE(pinningPosition, SQL_INTEGER)
			SQL_ATTRIBUTE(chatStateSendingEnabled, SQL_BOOL)
			SQL_LAST_ATTRIBUTE(readMarkerSendingEnabled, SQL_BOOL)
		)
	);

	execQuery(
		query,
		"INSERT INTO roster_tmp SELECT jid, name, subscription, encryption, unreadMessages, "
		"lastReadOwnMessageId, lastReadContactMessageId, readMarkerPending, pinningPosition, "
		"chatStateSendingEnabled, readMarkerSendingEnabled FROM roster"
	);

	execQuery(query, "UPDATE roster_tmp SET pinningPosition = -1 WHERE pinningPosition IS NULL");
	execQuery(query, "DROP TABLE roster");

	execQuery(
		query,
		SQL_CREATE_TABLE(
			"roster",
			SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(name, SQL_TEXT)
			SQL_ATTRIBUTE(subscription, SQL_INTEGER)
			SQL_ATTRIBUTE(encryption, SQL_INTEGER)
			SQL_ATTRIBUTE(unreadMessages, SQL_INTEGER)
			SQL_ATTRIBUTE(lastReadOwnMessageId, SQL_TEXT)
			SQL_ATTRIBUTE(lastReadContactMessageId, SQL_TEXT)
			SQL_ATTRIBUTE(readMarkerPending, SQL_BOOL)
			SQL_ATTRIBUTE(pinningPosition, SQL_INTEGER_NOT_NULL)
			SQL_ATTRIBUTE(chatStateSendingEnabled, SQL_BOOL)
			SQL_LAST_ATTRIBUTE(readMarkerSendingEnabled, SQL_BOOL)
		)
	);

	execQuery(query, "INSERT INTO roster SELECT * FROM roster_tmp");
	execQuery(query, "DROP TABLE roster_tmp");

	d->version = 26;
}

void Database::convertDatabaseToV27()
{
	DATABASE_CONVERT_TO_VERSION(26);
	QSqlQuery query(currentDatabase());
	execQuery(query, "ALTER TABLE roster ADD draftMessageId " SQL_TEXT " REFERENCES messages (id)");
	execQuery(query, "CREATE VIEW chatMessages AS SELECT * FROM messages WHERE deliveryState != 4");
	execQuery(query, "CREATE VIEW draftMessages AS SELECT * FROM messages WHERE deliveryState = 4");
	d->version = 27;
}

void Database::convertDatabaseToV28()
{
	DATABASE_CONVERT_TO_VERSION(27);
	QSqlQuery query(currentDatabase());
	execQuery(query, "ALTER TABLE roster ADD notificationsMuted " SQL_BOOL);
	d->version = 28;
}

void Database::convertDatabaseToV29()
{
	DATABASE_CONVERT_TO_VERSION(28);
	QSqlQuery query(currentDatabase());

	// Add the column "deliveryState" and remove the column "timestamp" from the primary key.
	execQuery(
		query,
		SQL_CREATE_TABLE(
			"messageReactions_tmp",
			SQL_ATTRIBUTE(messageSender, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(messageRecipient, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(messageId, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(senderJid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(emoji, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(timestamp, SQL_INTEGER)
			SQL_ATTRIBUTE(deliveryState, SQL_INTEGER)
			"PRIMARY KEY(messageSender, messageRecipient, messageId, senderJid, emoji)"
		)
	);

	execQuery(
		query,
		"INSERT INTO messageReactions_tmp SELECT messageSender, messageRecipient, messageId, "
		"senderJid, emoji, timestamp, NULL FROM messageReactions"
	);

	execQuery(query, "DROP TABLE messageReactions");

	execQuery(
		query,
		SQL_CREATE_TABLE(
			"messageReactions",
			SQL_ATTRIBUTE(messageSender, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(messageRecipient, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(messageId, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(senderJid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(emoji, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(timestamp, SQL_INTEGER)
			SQL_ATTRIBUTE(deliveryState, SQL_INTEGER)
			"PRIMARY KEY(messageSender, messageRecipient, messageId, senderJid, emoji)"
		)
	);

	execQuery(query, "INSERT INTO messageReactions SELECT * FROM messageReactions_tmp");
	execQuery(query, "DROP TABLE messageReactions_tmp");

	d->version = 29;
}

void Database::convertDatabaseToV30()
{
	DATABASE_CONVERT_TO_VERSION(29);
	QSqlQuery query(currentDatabase());
	execQuery(query, "ALTER TABLE messages ADD removed " SQL_BOOL_NOT_NULL " DEFAULT 0");
	execQuery(query, "DROP VIEW chatMessages");
	execQuery(query, "CREATE VIEW chatMessages AS SELECT * FROM messages WHERE deliveryState != 4 AND removed != 1");
	d->version = 30;
}

void Database::convertDatabaseToV31()
{
	DATABASE_CONVERT_TO_VERSION(30);
	QSqlQuery query(currentDatabase());
	execQuery(query, "ALTER TABLE messages RENAME COLUMN message TO body");
	d->version = 31;
}

void Database::convertDatabaseToV32()
{
	DATABASE_CONVERT_TO_VERSION(31);
	QSqlQuery query(currentDatabase());

	// Remove the column "isEdited".
	execQuery(
		query,
		SQL_CREATE_TABLE(
			"messages_tmp",
			SQL_ATTRIBUTE(sender, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(recipient, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(timestamp, SQL_TEXT)
			SQL_ATTRIBUTE(body, SQL_TEXT)
			SQL_ATTRIBUTE(id, SQL_TEXT)
			SQL_ATTRIBUTE(encryption, SQL_INTEGER)
			SQL_ATTRIBUTE(senderKey, SQL_BLOB)
			SQL_ATTRIBUTE(deliveryState, SQL_INTEGER)
			SQL_ATTRIBUTE(spoilerHint, SQL_TEXT)
			SQL_ATTRIBUTE(isSpoiler, SQL_BOOL)
			SQL_ATTRIBUTE(errorText, SQL_TEXT)
			SQL_ATTRIBUTE(replaceId, SQL_TEXT)
			SQL_ATTRIBUTE(originId, SQL_TEXT)
			SQL_ATTRIBUTE(stanzaId, SQL_TEXT)
			SQL_ATTRIBUTE(fileGroupId, SQL_INTEGER)
			SQL_ATTRIBUTE(removed, SQL_BOOL_NOT_NULL)
			"FOREIGN KEY(sender) REFERENCES roster (jid),"
			"FOREIGN KEY(recipient) REFERENCES roster (jid)"
		)
	);

	execQuery(
		query,
		"INSERT INTO messages_tmp SELECT sender, recipient, timestamp, body, id, encryption, "
		"senderKey, deliveryState, spoilerHint, isSpoiler, errorText, replaceId, "
		"originId, stanzaId, fileGroupId, removed FROM messages"
	);

	execQuery(query, "DROP TABLE messages");

	execQuery(
		query,
		SQL_CREATE_TABLE(
			"messages",
			SQL_ATTRIBUTE(sender, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(recipient, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(timestamp, SQL_TEXT)
			SQL_ATTRIBUTE(body, SQL_TEXT)
			SQL_ATTRIBUTE(id, SQL_TEXT)
			SQL_ATTRIBUTE(encryption, SQL_INTEGER)
			SQL_ATTRIBUTE(senderKey, SQL_BLOB)
			SQL_ATTRIBUTE(deliveryState, SQL_INTEGER)
			SQL_ATTRIBUTE(spoilerHint, SQL_TEXT)
			SQL_ATTRIBUTE(isSpoiler, SQL_BOOL)
			SQL_ATTRIBUTE(errorText, SQL_TEXT)
			SQL_ATTRIBUTE(replaceId, SQL_TEXT)
			SQL_ATTRIBUTE(originId, SQL_TEXT)
			SQL_ATTRIBUTE(stanzaId, SQL_TEXT)
			SQL_ATTRIBUTE(fileGroupId, SQL_INTEGER)
			SQL_ATTRIBUTE(removed, SQL_BOOL_NOT_NULL)
			"FOREIGN KEY(sender) REFERENCES roster (jid),"
			"FOREIGN KEY(recipient) REFERENCES roster (jid)"
		)
	);

	execQuery(query, "INSERT INTO messages SELECT * FROM messages_tmp");
	execQuery(query, "DROP TABLE messages_tmp");

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
	execQuery(query, "DROP TABLE roster");
	execQuery(
		query,
		SQL_CREATE_TABLE(
			"roster",
			SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(name, SQL_TEXT)
			SQL_ATTRIBUTE(subscription, SQL_INTEGER)
			SQL_ATTRIBUTE(encryption, SQL_INTEGER)
			SQL_ATTRIBUTE(unreadMessages, SQL_INTEGER)
			SQL_ATTRIBUTE(lastReadOwnMessageId, SQL_TEXT)
			SQL_ATTRIBUTE(lastReadContactMessageId, SQL_TEXT)
			SQL_ATTRIBUTE(readMarkerPending, SQL_BOOL)
			SQL_ATTRIBUTE(pinningPosition, SQL_INTEGER_NOT_NULL)
			SQL_ATTRIBUTE(chatStateSendingEnabled, SQL_BOOL)
			SQL_ATTRIBUTE(readMarkerSendingEnabled, SQL_BOOL)
			SQL_ATTRIBUTE(draftMessageId, SQL_TEXT)
			SQL_ATTRIBUTE(notificationsMuted, SQL_BOOL)
			"PRIMARY KEY(accountJid, jid),"
			"FOREIGN KEY(draftMessageId) REFERENCES messages (id)"
		)
	);

	d->version = 33;
}

void Database::convertDatabaseToV34()
{
	DATABASE_CONVERT_TO_VERSION(33);
	QSqlQuery query(currentDatabase());

	execQuery(
		query,
		SQL_CREATE_TABLE(
			"rosterGroups",
			SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(chatJid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(name, SQL_TEXT_NOT_NULL)
			"PRIMARY KEY(accountJid, chatJid, name),"
			"FOREIGN KEY(accountJid) REFERENCES roster (accountJid),"
			"FOREIGN KEY(chatJid) REFERENCES roster (jid)"
		)
	);

	d->version = 34;
}

void Database::convertDatabaseToV35()
{
	DATABASE_CONVERT_TO_VERSION(34);
	QSqlQuery query(currentDatabase());

	// Remove the column "draftMessageId".
	execQuery(
		query,
		SQL_CREATE_TABLE(
			"roster_tmp",
			SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(name, SQL_TEXT)
			SQL_ATTRIBUTE(subscription, SQL_INTEGER)
			SQL_ATTRIBUTE(encryption, SQL_INTEGER)
			SQL_ATTRIBUTE(unreadMessages, SQL_INTEGER)
			SQL_ATTRIBUTE(lastReadOwnMessageId, SQL_TEXT)
			SQL_ATTRIBUTE(lastReadContactMessageId, SQL_TEXT)
			SQL_ATTRIBUTE(readMarkerPending, SQL_BOOL)
			SQL_ATTRIBUTE(pinningPosition, SQL_INTEGER_NOT_NULL)
			SQL_ATTRIBUTE(chatStateSendingEnabled, SQL_BOOL)
			SQL_ATTRIBUTE(readMarkerSendingEnabled, SQL_BOOL)
			SQL_ATTRIBUTE(notificationsMuted, SQL_BOOL)
			"PRIMARY KEY(accountJid, jid)"
		)
	);

	execQuery(
		query,
		"INSERT INTO roster_tmp SELECT accountJid, jid, name, subscription, encryption, unreadMessages, "
		"lastReadOwnMessageId, lastReadContactMessageId, readMarkerPending, pinningPosition, "
		"chatStateSendingEnabled, readMarkerSendingEnabled, notificationsMuted FROM roster"
	);

	execQuery(query, "DROP TABLE roster");

	execQuery(
		query,
		SQL_CREATE_TABLE(
			"roster",
			SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(name, SQL_TEXT)
			SQL_ATTRIBUTE(subscription, SQL_INTEGER)
			SQL_ATTRIBUTE(encryption, SQL_INTEGER)
			SQL_ATTRIBUTE(unreadMessages, SQL_INTEGER)
			SQL_ATTRIBUTE(lastReadOwnMessageId, SQL_TEXT)
			SQL_ATTRIBUTE(lastReadContactMessageId, SQL_TEXT)
			SQL_ATTRIBUTE(readMarkerPending, SQL_BOOL)
			SQL_ATTRIBUTE(pinningPosition, SQL_INTEGER_NOT_NULL)
			SQL_ATTRIBUTE(chatStateSendingEnabled, SQL_BOOL)
			SQL_ATTRIBUTE(readMarkerSendingEnabled, SQL_BOOL)
			SQL_ATTRIBUTE(notificationsMuted, SQL_BOOL)
			"PRIMARY KEY(accountJid, jid)"
		)
	);

	execQuery(query, "INSERT INTO roster SELECT * FROM roster_tmp");
	execQuery(query, "DROP TABLE roster_tmp");

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
	execQuery(query, "DROP TABLE messages");
	execQuery(
		query,
		SQL_CREATE_TABLE(
			"messages",
			SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(chatJid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(senderId, SQL_TEXT)
			SQL_ATTRIBUTE(timestamp, SQL_TEXT)
			SQL_ATTRIBUTE(body, SQL_TEXT)
			SQL_ATTRIBUTE(id, SQL_TEXT)
			SQL_ATTRIBUTE(encryption, SQL_INTEGER)
			SQL_ATTRIBUTE(senderKey, SQL_BLOB)
			SQL_ATTRIBUTE(deliveryState, SQL_INTEGER)
			SQL_ATTRIBUTE(spoilerHint, SQL_TEXT)
			SQL_ATTRIBUTE(isSpoiler, SQL_BOOL)
			SQL_ATTRIBUTE(errorText, SQL_TEXT)
			SQL_ATTRIBUTE(replaceId, SQL_TEXT)
			SQL_ATTRIBUTE(originId, SQL_TEXT)
			SQL_ATTRIBUTE(stanzaId, SQL_TEXT)
			SQL_ATTRIBUTE(fileGroupId, SQL_INTEGER)
			SQL_ATTRIBUTE(removed, SQL_BOOL_NOT_NULL)
			"FOREIGN KEY(accountJid, chatJid) REFERENCES roster (accountJid, jid)"
		)
	);

	// Replace the columns "messageSender" and "messageRecipient" with "accountJid", "chatJid" and
	// "senderId".
	// Set a new primary key accordingly.
	// The values for the new columns cannot be determined by the database.
	// Thus, the table "messageReactions" is removed and recreated in order to store the latest
	// values from the server in the database again and include the values for the new columns.
	// Unfortunately, all data not stored on the server (e.g., "deliveryState") is lost.
	execQuery(query, "DROP TABLE messageReactions");
	execQuery(
		query,
		SQL_CREATE_TABLE(
			"messageReactions",
			SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(chatJid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(messageSenderId, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(messageId, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(senderJid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(emoji, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(timestamp, SQL_INTEGER)
			SQL_ATTRIBUTE(deliveryState, SQL_INTEGER)
			"PRIMARY KEY(accountJid, chatJid, messageSenderId, messageId, senderJid, emoji)"
		)
	);

	d->version = 36;
}

void Database::convertDatabaseToV37()
{
	DATABASE_CONVERT_TO_VERSION(36);
	QSqlQuery query(currentDatabase());

	// Reorder various columns for a consistent order through the whole code base.
	execQuery(
		query,
		SQL_CREATE_TABLE(
			"messages_tmp",
			SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(chatJid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(senderId, SQL_TEXT)
			SQL_ATTRIBUTE(id, SQL_TEXT)
			SQL_ATTRIBUTE(originId, SQL_TEXT)
			SQL_ATTRIBUTE(stanzaId, SQL_TEXT)
			SQL_ATTRIBUTE(replaceId, SQL_TEXT)
			SQL_ATTRIBUTE(timestamp, SQL_TEXT)
			SQL_ATTRIBUTE(body, SQL_TEXT)
			SQL_ATTRIBUTE(encryption, SQL_INTEGER)
			SQL_ATTRIBUTE(senderKey, SQL_BLOB)
			SQL_ATTRIBUTE(deliveryState, SQL_INTEGER)
			SQL_ATTRIBUTE(isSpoiler, SQL_BOOL)
			SQL_ATTRIBUTE(spoilerHint, SQL_TEXT)
			SQL_ATTRIBUTE(fileGroupId, SQL_INTEGER)
			SQL_ATTRIBUTE(errorText, SQL_TEXT)
			SQL_ATTRIBUTE(removed, SQL_BOOL_NOT_NULL)
			"FOREIGN KEY(accountJid, chatJid) REFERENCES roster (accountJid, jid)"
		)
	);

	execQuery(
		query,
		"INSERT INTO messages_tmp SELECT accountJid, chatJid, senderId, id, originId, stanzaId, "
		"replaceId, timestamp, body, encryption, senderKey, deliveryState, isSpoiler, spoilerHint, "
		"fileGroupId, errorText, removed FROM messages"
	);

	execQuery(query, "DROP TABLE messages");
	execQuery(
		query,
		SQL_CREATE_TABLE(
			"messages",
			SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(chatJid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(senderId, SQL_TEXT)
			SQL_ATTRIBUTE(id, SQL_TEXT)
			SQL_ATTRIBUTE(originId, SQL_TEXT)
			SQL_ATTRIBUTE(stanzaId, SQL_TEXT)
			SQL_ATTRIBUTE(replaceId, SQL_TEXT)
			SQL_ATTRIBUTE(timestamp, SQL_TEXT)
			SQL_ATTRIBUTE(body, SQL_TEXT)
			SQL_ATTRIBUTE(encryption, SQL_INTEGER)
			SQL_ATTRIBUTE(senderKey, SQL_BLOB)
			SQL_ATTRIBUTE(deliveryState, SQL_INTEGER)
			SQL_ATTRIBUTE(isSpoiler, SQL_BOOL)
			SQL_ATTRIBUTE(spoilerHint, SQL_TEXT)
			SQL_ATTRIBUTE(fileGroupId, SQL_INTEGER)
			SQL_ATTRIBUTE(errorText, SQL_TEXT)
			SQL_ATTRIBUTE(removed, SQL_BOOL_NOT_NULL)
			"FOREIGN KEY(accountJid, chatJid) REFERENCES roster (accountJid, jid)"
		)
	);

	execQuery(query, "INSERT INTO messages SELECT * FROM messages_tmp");
	execQuery(query, "DROP TABLE messages_tmp");

	d->version = 37;
}

void Database::convertDatabaseToV38()
{
	DATABASE_CONVERT_TO_VERSION(37);
	QSqlQuery query(currentDatabase());
	execQuery(query, "ALTER TABLE Roster ADD automaticMediaDownloadsRule " SQL_INTEGER);
	d->version = 38;
}

void Database::convertDatabaseToV39()
{
	DATABASE_CONVERT_TO_VERSION(38)
	QSqlQuery query(currentDatabase());

	// blocked
	execQuery(
		query,
		SQL_CREATE_TABLE(
			"blocked",
			SQL_ATTRIBUTE(accountJid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL)
			"PRIMARY KEY(accountJid, jid)"
		)
	);

	d->version = 39;
}

void Database::convertDatabaseToV40()
{
	DATABASE_CONVERT_TO_VERSION(39)
	QSqlQuery query(currentDatabase());

	execQuery(
		query,
		SQL_CREATE_TABLE(
			"accounts",
			SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(name, SQL_TEXT)
			SQL_ATTRIBUTE(latestMessageStanzaId, SQL_TEXT)
			SQL_ATTRIBUTE(latestMessageTimestamp, SQL_TEXT)
			"PRIMARY KEY(jid)"
		)
	);

	d->version = 40;
}
