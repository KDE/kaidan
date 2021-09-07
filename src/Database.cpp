/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2021 Kaidan developers and contributors
 *  (see the LICENSE file for a full list of copyright authors)
 *
 *  Kaidan is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  In addition, as a special exception, the author of Kaidan gives
 *  permission to link the code of its release with the OpenSSL
 *  project's "OpenSSL" library (or with modified versions of it that
 *  use the same license as the "OpenSSL" library), and distribute the
 *  linked executables. You must obey the GNU General Public License in
 *  all respects for all of the code used other than "OpenSSL". If you
 *  modify this file, you may extend this exception to your version of
 *  the file, but you are not obligated to do so.  If you do not wish to
 *  do so, delete this exception statement from your version.
 *
 *  Kaidan is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kaidan.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Database.h"
#include "Globals.h"
#include "Utils.h"

#include <QDir>
#include <QMutex>
#include <QRandomGenerator>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlField>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QThreadStorage>
#include <QThreadPool>

#include "Kaidan.h"

#define DATABASE_CONVERT_TO_VERSION(n) \
	if (d->version < n) { \
		convertDatabaseToV##n(); \
	}

// Both need to be updated on version bump:
#define DATABASE_LATEST_VERSION 13
#define DATABASE_CONVERT_TO_LATEST_VERSION() DATABASE_CONVERT_TO_VERSION(13)

#define SQL_BOOL "BOOL"
#define SQL_INTEGER "INTEGER"
#define SQL_INTEGER_NOT_NULL "INTEGER NOT NULL"
#define SQL_TEXT "TEXT"
#define SQL_TEXT_NOT_NULL "TEXT NOT NULL"
#define SQL_BLOB "BLOB"

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

		const auto writeDir = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
		if (!writeDir.mkpath(QLatin1String("."))) {
			qFatal("Failed to create writable directory at %s", qPrintable(writeDir.absolutePath()));
		}

		// Ensure that we have a writable location on all devices.
		const auto fileName = writeDir.absoluteFilePath(QStringLiteral(DB_FILENAME));
		// open() will create the SQLite database if it doesn't exist.
		database.setDatabaseName(fileName);
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
	QThreadPool pool;
	QMutex tableCreationMutex;
	int version = DbNotLoaded;
	int transactions = 0;
	bool tablesCreated = false;
};

Database::Database(QObject *parent)
	: QObject(parent),
	  d(new DatabasePrivate)
{
	d->pool.setMaxThreadCount(1);
	d->pool.setExpiryTimeout(-1);
	connect(this, &Database::transactionRequested, this, &Database::transaction);
	connect(this, &Database::commitRequested, this, &Database::commit);
}

Database::~Database()
{
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
	Q_ASSERT(transactions >= 0);

	if (!transactions) {
		// no transaction requested anymore
		auto db = currentDatabase();
		if (!db.commit()) {
			qWarning() << "Could not commit transaction on database:"
			           << db.lastError().text();
		}
	}
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
		if (tables.contains(DB_TABLE_MESSAGES) &&
			tables.contains(DB_TABLE_ROSTER)) {
			// old Kaidan v0.1/v0.2 table
			d->version = DbOldVersion;
		} else {
			d->version = DbNotCreated;
		}
		// we've got all we want; do not query for a db version
		return;
	}

	QSqlQuery query(db);
	Utils::execQuery(query, "SELECT version FROM " DB_TABLE_INFO);

	QSqlRecord record = query.record();
	int versionCol = record.indexOf("version");

	while (query.next()) {
		d->version = query.value(versionCol).toInt();
	}
}

void Database::saveDatabaseInfo()
{
	if (d->version <= DbOldVersion || d->version > DATABASE_LATEST_VERSION) {
		qFatal("[database] Fatal error: Attempted to save invalid db version number.");
	}

	QSqlRecord updateRecord;
	updateRecord.append(Utils::createSqlField("version", d->version));

	auto db = currentDatabase();
	QSqlQuery query(db);
	Utils::execQuery(
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

QThreadPool *Database::threadPool()
{
	return &d->pool;
}

bool Database::needToConvert()
{
	return d->version < DATABASE_LATEST_VERSION;
}

void Database::convertDatabase()
{
	qDebug() << "[database] Converting database to latest version from version" << d->version;
	transaction();

	if (d->version == DbNotCreated)
		createNewDatabase();
	else
		DATABASE_CONVERT_TO_LATEST_VERSION();

	saveDatabaseInfo();
	commit();
}

void Database::createNewDatabase()
{
	createDbInfoTable();
	createRosterTable();
	createMessagesTable();

	d->version = DATABASE_LATEST_VERSION;
}

void Database::createDbInfoTable()
{
	auto db = currentDatabase();
	QSqlQuery query(db);
	Utils::execQuery(
		query,
		SQL_CREATE_TABLE(
			DB_TABLE_INFO,
			SQL_LAST_ATTRIBUTE(version, SQL_INTEGER_NOT_NULL)
		)
	);

	QSqlRecord insertRecord;
	insertRecord.append(Utils::createSqlField("version", DATABASE_LATEST_VERSION));
	Utils::execQuery(
		query,
		db.driver()->sqlStatement(
			QSqlDriver::InsertStatement,
			DB_TABLE_INFO,
			insertRecord,
			false
		)
	);
}

void Database::createRosterTable()
{
	// TODO: remove lastExchanged and lastMessage

	QSqlQuery query(currentDatabase());
	Utils::execQuery(
		query,
		SQL_CREATE_TABLE(
			DB_TABLE_ROSTER,
			SQL_ATTRIBUTE(jid, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(name, SQL_TEXT)
			SQL_ATTRIBUTE(lastExchanged, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(unreadMessages, SQL_INTEGER)
			SQL_LAST_ATTRIBUTE(lastMessage, SQL_TEXT)
		)
	);
}

void Database::createMessagesTable()
{
	// TODO: the next time we change the messages table, we need to do:
	//  * rename author to sender, edited to isEdited
	//  * delete author_resource, recipient_resource
	//  * remove 'NOT NULL' from id
	//  * remove columns isSent, isDelivered

	QSqlQuery query(currentDatabase());
	Utils::execQuery(
		query,
		SQL_CREATE_TABLE(
			DB_TABLE_MESSAGES,
			SQL_ATTRIBUTE(author, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(author_resource, SQL_TEXT)
			SQL_ATTRIBUTE(recipient, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(recipient_resource, SQL_TEXT)
			SQL_ATTRIBUTE(timestamp, SQL_TEXT)
			SQL_ATTRIBUTE(message, SQL_TEXT)
			SQL_ATTRIBUTE(id, SQL_TEXT_NOT_NULL)
			SQL_ATTRIBUTE(isSent, SQL_BOOL)
			SQL_ATTRIBUTE(isDelivered, SQL_BOOL)
			SQL_ATTRIBUTE(deliveryState, SQL_INTEGER)
			SQL_ATTRIBUTE(type, SQL_INTEGER)
			SQL_ATTRIBUTE(mediaUrl, SQL_TEXT)
			SQL_ATTRIBUTE(mediaSize, SQL_INTEGER)
			SQL_ATTRIBUTE(mediaContentType, SQL_TEXT)
			SQL_ATTRIBUTE(mediaLastModified, SQL_INTEGER)
			SQL_ATTRIBUTE(mediaLocation, SQL_TEXT)
			SQL_ATTRIBUTE(mediaThumb, SQL_BLOB)
			SQL_ATTRIBUTE(mediaHashes, SQL_TEXT)
			SQL_ATTRIBUTE(edited, SQL_BOOL)
			SQL_ATTRIBUTE(spoilerHint, SQL_TEXT)
			SQL_ATTRIBUTE(isSpoiler, SQL_BOOL)
			SQL_ATTRIBUTE(errorText, SQL_TEXT)
			SQL_ATTRIBUTE(replaceId, SQL_TEXT)
			SQL_ATTRIBUTE(originId, SQL_TEXT)
			SQL_ATTRIBUTE(stanzaId, SQL_TEXT)
			"FOREIGN KEY(author) REFERENCES " DB_TABLE_ROSTER " (jid),"
			"FOREIGN KEY(recipient) REFERENCES " DB_TABLE_ROSTER " (jid)"
		)
	);
}

void Database::convertDatabaseToV2()
{
	// create a new dbinfo table
	createDbInfoTable();
	d->version = 2;
}

void Database::convertDatabaseToV3()
{
	DATABASE_CONVERT_TO_VERSION(2);
	QSqlQuery query(currentDatabase());
	Utils::execQuery(query, "ALTER TABLE Roster ADD avatarHash " SQL_TEXT);
	d->version = 3;
}

void Database::convertDatabaseToV4()
{
	DATABASE_CONVERT_TO_VERSION(3);
	QSqlQuery query(currentDatabase());
	// SQLite doesn't support the ALTER TABLE drop columns feature, so we have to use a workaround.
	// we copy all rows into a back-up table (but without `avatarHash`), and then delete the old table
	// and copy everything to the normal table again
	Utils::execQuery(query, "CREATE TEMPORARY TABLE roster_backup(jid, name, lastExchanged,"
							"unreadMessages, lastMessage, lastOnline, activity, status, mood)");
	Utils::execQuery(query, "INSERT INTO roster_backup SELECT jid, name, lastExchanged, unreadMessages,"
							"lastMessage, lastOnline, activity, status, mood FROM " DB_TABLE_ROSTER);
	Utils::execQuery(query, "DROP TABLE Roster");
	Utils::execQuery(query, "CREATE TABLE Roster (jid " SQL_TEXT_NOT_NULL ", name " SQL_TEXT_NOT_NULL ","
							"lastExchanged " SQL_TEXT_NOT_NULL ", unreadMessages " SQL_INTEGER ", lastMessage  " SQL_TEXT ","
							"lastOnline " SQL_TEXT ", activity " SQL_TEXT ", status " SQL_TEXT ", mood " SQL_TEXT ")");
	Utils::execQuery(query, "INSERT INTO Roster SELECT jid, name, lastExchanged, unreadMessages,"
							"lastMessage, lastOnline, activity, status, mood FROM Roster_backup");
	Utils::execQuery(query, "DROP TABLE Roster_backup");
	d->version = 4;
}

void Database::convertDatabaseToV5()
{
	DATABASE_CONVERT_TO_VERSION(4);
	QSqlQuery query(currentDatabase());
	Utils::execQuery(query, "ALTER TABLE Messages ADD type " SQL_INTEGER);
	Utils::execQuery(query, "UPDATE Messages SET type = 0 WHERE type IS NULL");
	Utils::execQuery(query, "ALTER TABLE Messages ADD mediaUrl " SQL_TEXT);
	d->version = 5;
}

void Database::convertDatabaseToV6()
{
	DATABASE_CONVERT_TO_VERSION(5);
	QSqlQuery query(currentDatabase());
	for (const QString &column : {	"mediaSize " SQL_INTEGER,
									"mediaContentType " SQL_TEXT,
									"mediaLastModified " SQL_INTEGER,
									"mediaLocation " SQL_TEXT }) {
		Utils::execQuery(query, QString("ALTER TABLE Messages ADD ").append(column));
	}
	d->version = 6;
}

void Database::convertDatabaseToV7()
{
	DATABASE_CONVERT_TO_VERSION(6);
	QSqlQuery query(currentDatabase());
	Utils::execQuery(query, "ALTER TABLE Messages ADD mediaThumb " SQL_BLOB);
	Utils::execQuery(query, "ALTER TABLE Messages ADD mediaHashes " SQL_TEXT);
	d->version = 7;
}

void Database::convertDatabaseToV8()
{
	DATABASE_CONVERT_TO_VERSION(7);
	QSqlQuery query(currentDatabase());
	Utils::execQuery(query, "CREATE TEMPORARY TABLE roster_backup(jid, name, lastExchanged, unreadMessages, lastMessage)");
	Utils::execQuery(query, "INSERT INTO roster_backup SELECT jid, name, lastExchanged, unreadMessages, lastMessage FROM Roster");
	Utils::execQuery(query, "DROP TABLE Roster");
	Utils::execQuery(
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

	Utils::execQuery(query, "INSERT INTO Roster SELECT jid, name, lastExchanged, unreadMessages, lastMessage FROM Roster_backup");
	Utils::execQuery(query, "DROP TABLE roster_backup");
	d->version = 8;
}

void Database::convertDatabaseToV9()
{
	DATABASE_CONVERT_TO_VERSION(8);
	QSqlQuery query(currentDatabase());
	Utils::execQuery(query, "ALTER TABLE Messages ADD edited " SQL_BOOL);
	d->version = 9;
}

void Database::convertDatabaseToV10()
{
	DATABASE_CONVERT_TO_VERSION(9);
	QSqlQuery query(currentDatabase());
	Utils::execQuery(query, "ALTER TABLE Messages ADD isSpoiler " SQL_BOOL);
	Utils::execQuery(query, "ALTER TABLE Messages ADD spoilerHint " SQL_TEXT);
	d->version = 10;
}

void Database::convertDatabaseToV11()
{
	DATABASE_CONVERT_TO_VERSION(10);
	QSqlQuery query(currentDatabase());
	Utils::execQuery(query, "ALTER TABLE Messages ADD deliveryState " SQL_INTEGER);
	Utils::execQuery(query, "UPDATE Messages SET deliveryState = 2 WHERE isDelivered = 1");
	Utils::execQuery(query, "UPDATE Messages SET deliveryState = 1 WHERE deliveryState IS NULL");
	Utils::execQuery(query, "ALTER TABLE Messages ADD errorText " SQL_TEXT);
	d->version = 11;
}

void Database::convertDatabaseToV12()
{
	DATABASE_CONVERT_TO_VERSION(11);
	QSqlQuery query(currentDatabase());
	Utils::execQuery(query, "ALTER TABLE Messages ADD replaceId " SQL_TEXT);
	d->version = 12;
}

void Database::convertDatabaseToV13()
{
	DATABASE_CONVERT_TO_VERSION(12);
	QSqlQuery query(currentDatabase());
	Utils::execQuery(query, "ALTER TABLE Messages ADD stanzaId " SQL_TEXT);
	Utils::execQuery(query, "ALTER TABLE Messages ADD originId " SQL_TEXT);
	d->version = 13;
}
