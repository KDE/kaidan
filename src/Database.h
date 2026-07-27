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

// std
#include <memory>
#include <span>
// Qt
#include <QObject>

class QSqlQuery;
class QSqlDatabase;
struct DatabasePrivate;

/**
 * The Database class manages the SQL database. It opens the database and converts old
 * formats.
 */
class Database : public QObject
{
    Q_OBJECT

public:
    static Database *instance();

    explicit Database(QObject *parent = nullptr);
    ~Database();

    /**
     * Converts the database to the latest version and guarantees that all tables have
     * been created.
     */
    void createTables();

    // used in unit tests
    void createV3Database();

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

    /**
     * One step of the database schema history.
     */
    struct Migration {
        /**
         * Schema version the database has once @c apply has run.
         */
        int version;

        /**
         * A plain function pointer on purpose: a migration cannot capture anything and
         * cannot reach into the Database instance.
         */
        void (*apply)(QSqlQuery &query);
    };

    /**
     * All migrations, ordered by strictly increasing version.
     */
    static std::span<const Migration> migrations();

    static int latestVersion();

    std::unique_ptr<DatabasePrivate> d;

    static Database *s_instance;

    friend class DatabaseComponent;
    friend class DatabaseTest;
    // TODO: Remove TrustDb as friend class once MessageDb is no singleton anymore
    friend class TrustDb;
};
