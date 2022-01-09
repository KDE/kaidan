/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2022 Kaidan developers and contributors
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

#include "DatabaseComponent.h"

#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QSqlRecord>

#include "Database.h"

DatabaseComponent::DatabaseComponent(Database *database, QObject *parent)
	: QObject(parent),
	  m_database(database)
{
}

QSqlQuery DatabaseComponent::createQuery()
{
	return m_database->createQuery();
}

QSqlDriver &DatabaseComponent::sqlDriver()
{
	return *m_database->currentDatabase().driver();
}

QSqlRecord DatabaseComponent::sqlRecord(const QString &tableName)
{
	return m_database->currentDatabase().record(tableName);
}

void DatabaseComponent::transaction()
{
	m_database->transaction();
}

void DatabaseComponent::commit()
{
	m_database->commit();
}

QThreadPool *DatabaseComponent::threadPool() const
{
	return m_database->threadPool();
}
