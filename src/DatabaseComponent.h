// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QObject>
// Kaidan
#include "FutureUtils.h"

class QThreadPool;
class QSqlQuery;
class QSqlDriver;
class QSqlRecord;
class Database;

class DatabaseComponent : public QObject
{
	Q_OBJECT
public:
	DatabaseComponent(Database *database, QObject *parent = nullptr);

	QSqlQuery createQuery();
	QSqlDriver &sqlDriver();
	QSqlRecord sqlRecord(const QString &tableName);
	void transaction();
	void commit();

	template<typename Functor>
	auto run(Functor function) const
	{
		return runAsync(dbWorker(), function);
	}

protected:
	QObject *dbWorker() const;

private:
	Database *m_database;
};
