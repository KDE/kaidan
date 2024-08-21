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

QFuture<Account> AccountDb::account(const QString &jid)
{
	return run([this, jid]() {
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

		return accounts.first();
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

QFuture<qint64> AccountDb::fetchHttpUploadLimit(const QString &jid)
{
	return run([this, jid]() {
		auto query = createQuery();
		execQuery(
			query,
			QStringLiteral(R"(
				SELECT httpUploadLimit
				FROM accounts
				WHERE jid = :jid
			)"),
			{
				{ u":jid", jid },
			}
		);

		qint64 size = 0;

		if (query.first()) {
			size = query.value(0).toLongLong();
		}

		return size;
	});
}

QFuture<void> AccountDb::removeAccount(const QString &jid)
{
	return run([this, jid]() {
		auto query = createQuery();
		execQuery(
			query,
			QStringLiteral(R"(
				DELETE FROM accounts WHERE jid = :jid
			)"),
			{
				{ u":jid", jid },
			}
		);
	});
}

void AccountDb::parseAccountsFromQuery(QSqlQuery &query, QVector<Account> &accounts)
{
	QSqlRecord rec = query.record();

	int idxJid = rec.indexOf(QStringLiteral("jid"));
	int idxName = rec.indexOf(QStringLiteral("name"));
	int idxLatestMessageStanzaId = rec.indexOf(QStringLiteral("latestMessageStanzaId"));
	int idxLatestMessageStanzaTimestamp = rec.indexOf(QStringLiteral("latestMessageStanzaTimestamp"));
	int idxHttpUploadLimit = rec.indexOf(QStringLiteral("httpUploadLimit"));
	int idxContactNotificationRule = rec.indexOf(QStringLiteral("contactNotificationRule"));
	int idxGroupChatNotificationRule = rec.indexOf(QStringLiteral("groupChatNotificationRule"));

	reserve(accounts, query);
	while (query.next()) {
		Account account;

		account.jid = query.value(idxJid).toString();
		account.name = query.value(idxName).toString();
		account.latestMessageStanzaId = query.value(idxLatestMessageStanzaId).toString();
		account.latestMessageStanzaTimestamp = query.value(idxLatestMessageStanzaTimestamp).toDateTime();
		account.httpUploadLimit = query.value(idxHttpUploadLimit).toLongLong();

		if (const auto contactNotificationRule = query.value(idxContactNotificationRule);
			!contactNotificationRule.isNull()) {
			account.contactNotificationRule = query.value(idxContactNotificationRule).value<Account::ContactNotificationRule>();
		}

		if (const auto groupChatNotificationRule = query.value(idxGroupChatNotificationRule);
			!groupChatNotificationRule.isNull()) {
			account.groupChatNotificationRule = query.value(idxGroupChatNotificationRule).value<Account::GroupChatNotificationRule>();
		}

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
	if (oldAccount.latestMessageStanzaTimestamp != newAccount.latestMessageStanzaTimestamp) {
		rec.append(createSqlField(QStringLiteral("latestMessageStanzaTimestamp"), newAccount.latestMessageStanzaTimestamp));
	}
	if (oldAccount.httpUploadLimit != newAccount.httpUploadLimit) {
		rec.append(createSqlField(QStringLiteral("httpUploadLimit"), newAccount.httpUploadLimit));
	}
	if (oldAccount.contactNotificationRule != newAccount.contactNotificationRule) {
		rec.append(createSqlField(QStringLiteral("contactNotificationRule"), static_cast<int>(newAccount.contactNotificationRule)));
	}
	if (oldAccount.groupChatNotificationRule != newAccount.groupChatNotificationRule) {
		rec.append(createSqlField(QStringLiteral("groupChatNotificationRule"), static_cast<int>(newAccount.groupChatNotificationRule)));
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
