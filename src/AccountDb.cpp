// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
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
		execQuery(
			query,
			QStringLiteral(R"(
				INSERT OR IGNORE INTO accounts (
					jid
				)
				VALUES (
					:jid
				)
			)"),
			{
				{ u":jid", jid },
			}
		);
	});
}

QFuture<void> AccountDb::updateAccount(const QString &jid, const std::function<void (Account &)> &updateAccount)
{
	return run([this, jid, updateAccount]() {
		// Load the account from the database.
		auto query = createQuery();
		execQuery(
			query,
			QStringLiteral(R"(
				SELECT *
				FROM accounts
				WHERE jid = :jid
			)"),
			{
				{ u":jid", jid },
			}
		);

		QVector<Account> accounts;
		parseAccountsFromQuery(query, accounts);

		// Update the loaded account.
		if (!accounts.isEmpty()) {
			const auto &oldAccount = accounts.constFirst();
			Account newAccount = oldAccount;
			updateAccount(newAccount);

			// Replace the old account's values with the updated ones if the item has changed.
			if (oldAccount != newAccount) {
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
		execQuery(
			query,
			QStringLiteral(R"(
				SELECT latestMessageStanzaId
				FROM accounts
				WHERE jid = :jid
			)"),
			{
				{ u":jid", jid },
			}
		);

		QString stanzaId;

		if (query.first()) {
			stanzaId = query.value(0).toString();
		}

		return stanzaId;
	});
}

void AccountDb::parseAccountsFromQuery(QSqlQuery &query, QVector<Account> &accounts)
{
	QSqlRecord rec = query.record();

	int idxJid = rec.indexOf(QStringLiteral("jid"));
	int idxName = rec.indexOf(QStringLiteral("name"));
	int idxLatestMessageStanzaId = rec.indexOf(QStringLiteral("latestMessageStanzaId"));
	int idxLatestMessageTimestamp = rec.indexOf(QStringLiteral("latestMessageTimestamp"));

	reserve(accounts, query);
	while (query.next()) {
		Account account;

		account.jid = query.value(idxJid).toString();
		account.name = query.value(idxName).toString();
		account.latestMessageStanzaId = query.value(idxLatestMessageStanzaId).toString();
		account.latestMessageTimestamp = query.value(idxLatestMessageTimestamp).toDateTime();

		accounts << std::move(account);
	}
}

QSqlRecord AccountDb::createUpdateRecord(const Account &oldAccount, const Account &newAccount)
{
	QSqlRecord rec;

	if (oldAccount.jid != newAccount.jid) {
		rec.append(createSqlField(QStringLiteral("jid"), newAccount.jid));
	}
	if (oldAccount.name != newAccount.name) {
		rec.append(createSqlField(QStringLiteral("name"), newAccount.name));
	}
	if (oldAccount.latestMessageStanzaId != newAccount.latestMessageStanzaId) {
		rec.append(createSqlField(QStringLiteral("latestMessageStanzaId"), newAccount.latestMessageStanzaId));
	}
	if (oldAccount.latestMessageTimestamp != newAccount.latestMessageTimestamp) {
		rec.append(createSqlField(QStringLiteral("latestMessageTimestamp"), newAccount.latestMessageTimestamp));
	}

	return rec;
}

void AccountDb::updateAccountByRecord(const QString &jid, const QSqlRecord &record)
{
	auto query = createQuery();
	auto &driver = sqlDriver();

	QMap<QString, QVariant> keyValuePairs = {
		{ QStringLiteral("jid"), jid }
	};

	execQuery(
		query,
		driver.sqlStatement(
			QSqlDriver::UpdateStatement,
			QStringLiteral(DB_TABLE_ACCOUNTS),
			record,
			false
		) +
		simpleWhereStatement(&driver, keyValuePairs)
	);
}
