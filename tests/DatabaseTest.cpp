// SPDX-FileCopyrightText: 2024 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

// std
#include <ranges>
// Qt
#include <QSqlDatabase>
#include <QSqlField>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTest>
// Kaidan
#include "Algorithms.h"
#include "Database.h"
#include "Settings.h"
#include "SqlUtils.h"
#include "Test.h"

using namespace SqlUtils;
using std::ranges::sort;
using std::views::iota;
using std::views::zip;

static auto dbTableRecords(QSqlDatabase &db)
{
    auto tables = db.tables();
    std::sort(tables.begin(), tables.end());

    auto records = transform(tables, [&](const auto &tableName) {
        return db.record(tableName);
    });

    return std::tuple{std::move(tables), std::move(records)};
}

static auto fieldNames(const QSqlRecord &record)
{
    QStringList fieldNames;
    for (int i : iota(0, record.count())) {
        fieldNames.push_back(record.fieldName(i));
    }
    return fieldNames;
}

class DatabaseTest : public Test
{
    Q_OBJECT

private:
    Q_SLOT void conversion();
};

void DatabaseTest::conversion()
{
    new Settings(this);
    Database db;
    auto sqlDb = db.currentDatabase();

    // create database using converion functions
    db.createV3Database();
    db.createTables();

    // dump tables
    auto [tableNamesConversion, recordsConversion] = dbTableRecords(sqlDb);

    // remove all created tables
    QSqlQuery query(sqlDb);
    for (const auto &tableName : sqlDb.tables()) {
        execQuery(query, QStringLiteral("DROP TABLE ") + tableName);
    }
    for (const auto &viewName : sqlDb.tables(QSql::Views)) {
        execQuery(query, QStringLiteral("DROP VIEW ") + viewName);
    }

    // create tables directly (without conversion functions)
    db.createNewDatabase();

    // dump tables
    auto [tableNamesNew, recordsNew] = dbTableRecords(sqlDb);

    // check the same tables exist in both cases
    QCOMPARE(tableNamesConversion, tableNamesNew);

    Q_ASSERT(tableNamesConversion.size() == recordsConversion.size());
    Q_ASSERT(tableNamesNew.size() == recordsNew.size());

    // check records of each table
    for (const auto &[tableName, recordC, recordN] : zip(tableNamesConversion, recordsConversion, recordsNew)) {
        auto fieldNamesC = fieldNames(recordC);
        auto fieldNamesN = fieldNames(recordN);

        bool orderMatches = (fieldNamesC == fieldNamesN);

        // compare without order
        std::sort(fieldNamesC.begin(), fieldNamesC.end());
        std::sort(fieldNamesN.begin(), fieldNamesN.end());
        QCOMPARE(fieldNamesC.join(u", "), fieldNamesN.join(u", "));

        // print warning if order does not match
        if (!orderMatches) {
            qWarning() << "Order of fields of" << tableName << "does not match between conversion/new variant.";
            qWarning() << "  " << fieldNames(recordC);
            qWarning() << "  " << fieldNames(recordN);
        }

        // check optional/required property of each field
        for (const auto &fieldName : fieldNamesC) {
            auto fieldC = recordC.field(recordC.indexOf(fieldName));
            auto fieldN = recordN.field(recordN.indexOf(fieldName));

            // check 'NOT NULL'
            if (fieldC.requiredStatus() != fieldN.requiredStatus()) {
                qDebug() << tableName << fieldName << "has wrong required status (NOT NULL)!";
                qDebug() << "  Conversion variant is:" << fieldC.requiredStatus();
                qDebug() << "  New variant is:" << fieldN.requiredStatus();
                QFAIL("Field required status of conversion/new variant does not match.");
            }

            // check data type
            if (fieldC.metaType() != fieldN.metaType()) {
                qDebug() << tableName << fieldName << "data type mismatch!";
                qDebug() << "  Conversion variant:" << fieldC.metaType();
                qDebug() << "  New variant is:" << fieldN.metaType();
                QFAIL("Field data type of conversion/new variant does not match.");
            }
        }
    }
}

QTEST_GUILESS_MAIN(DatabaseTest)
#include "DatabaseTest.moc"
