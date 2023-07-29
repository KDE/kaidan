// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Xavier <xavi@delape.net>
// SPDX-FileCopyrightText: 2020 Yury Gubich <blue@macaw.me>
// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <QObject>

class QSqlQuery;
class QSqlDatabase;
class QThreadPool;
struct DatabasePrivate;

/**
 * The Database class manages the SQL database. It opens the database and converts old
 * formats.
 */
class Database : public QObject
{
	Q_OBJECT
	friend class DatabaseComponent;

public:
	Database(QObject *parent = nullptr);
	~Database();

	/**
	 * Converts the database to the latest version and guarantees that all tables have
	 * been created.
	 */
	void createTables();

	/// Transaction on random thread from the thread pool (should be replaced in the
	/// future).
	void startTransaction();
	void commitTransaction();

private:
	QObject *dbWorker() const;
	QSqlDatabase currentDatabase();
	QSqlQuery createQuery();

	/// Returns the number of active transactions on the current thread.
	int &activeTransactions();
	/// Begins a transaction if none has been started.
	void transaction();
	/// Commits the transaction if every transaction has been finished.
	void commit();

	/**
	 * @return true if the database has to be converted using @c convertDatabase()
	 * because the database is not up-to-date.
	 */
	bool needToConvert();

	/**
	 * Converts the database to latest model.
	 */
	void convertDatabase();

	/**
	 * Loads the database information and detects the database version.
	 */
	void loadDatabaseInfo();

	/**
	 * Saves the database information.
	 */
	void saveDatabaseInfo();

	/**
	 * Creates a new database without content.
	 */
	void createNewDatabase();

	/*
	 * Upgrades the database to the next version.
	 */
	void convertDatabaseToV2();
	void convertDatabaseToV3();
	void convertDatabaseToV4();
	void convertDatabaseToV5();
	void convertDatabaseToV6();
	void convertDatabaseToV7();
	void convertDatabaseToV8();
	void convertDatabaseToV9();
	void convertDatabaseToV10();
	void convertDatabaseToV11();
	void convertDatabaseToV12();
	void convertDatabaseToV13();
	void convertDatabaseToV14();
	void convertDatabaseToV15();
	void convertDatabaseToV16();
	void convertDatabaseToV17();
	void convertDatabaseToV18();
	void convertDatabaseToV19();
	void convertDatabaseToV20();
	void convertDatabaseToV21();
	void convertDatabaseToV22();
	void convertDatabaseToV23();
	void convertDatabaseToV24();
	void convertDatabaseToV25();
	void convertDatabaseToV26();
	void convertDatabaseToV27();
	void convertDatabaseToV28();
	void convertDatabaseToV29();
	void convertDatabaseToV30();
	void convertDatabaseToV31();
	void convertDatabaseToV32();
	void convertDatabaseToV33();
	void convertDatabaseToV34();

	std::unique_ptr<DatabasePrivate> d;
};
