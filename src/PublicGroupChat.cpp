// SPDX-FileCopyrightText: 2022 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PublicGroupChat.h"
#include "Algorithms.h"
#include "JsonUtils.h"

static QStringList splitString(const QString &string, QString seps)
{
	static const QString quotes = QStringLiteral("'\"");
	const QStringList parts = string.split(seps.back());
	QStringList result;

	seps.chop(1);
	result.reserve(parts.count());

	for (const QString &part : parts) {
		QString trimmed = part.trimmed();

		if (!trimmed.isEmpty()) {
			if (seps.isEmpty()) {
				for (const QChar &quote : quotes) {
					if (trimmed.startsWith(quote) && trimmed.endsWith(quote)) {
						trimmed.remove(0, 1);
						trimmed.chop(1);
					}
				}

				result.append(trimmed);
			} else {
				result.append(splitString(trimmed, seps));
			}
		}
	}

	return result;
}

PublicGroupChat::PublicGroupChat(const QJsonObject &object)
{
	Q_ASSERT(!object.isEmpty());

	m_address = JSON_VALUE(QString, Address);
	m_users = JSON_VALUE(int, Users);
	m_isOpen = JSON_VALUE(bool, IsOpen);
	m_name = JSON_VALUE(QString, Name);
	m_description = JSON_VALUE(QString, Description);
	m_languages = splitLanguages(JSON_VALUE(QString, Language));
}

PublicGroupChat::PublicGroupChat(const PublicGroupChat &other) = default;

const QString &PublicGroupChat::address() const
{
	return m_address;
}

void PublicGroupChat::setAddress(const QString &address)
{
	m_address = address;
}

int PublicGroupChat::users() const
{
	return m_users;
}

void PublicGroupChat::setUsers(int users)
{
	m_users = users;
}

bool PublicGroupChat::isOpen() const
{
	return m_isOpen;
}

void PublicGroupChat::setIsOpen(bool isOpen)
{
	m_isOpen = isOpen;
}

const QString &PublicGroupChat::name() const
{
	return m_name;
}

void PublicGroupChat::setName(const QString &name)
{
	m_name = name;
}

const QString &PublicGroupChat::description() const
{
	return m_description;
}

void PublicGroupChat::setDescription(const QString &description)
{
	m_description = description;
}

const QStringList &PublicGroupChat::languages() const
{
	return m_languages;
}

void PublicGroupChat::setLanguages(const QStringList &languages)
{
	m_languages = languages;
}

PublicGroupChat &PublicGroupChat::operator=(const PublicGroupChat &other) = default;

QJsonObject PublicGroupChat::toJson() const
{
	QJsonObject object;
	ADD_JSON_VALUE(Address, m_address);
	ADD_JSON_VALUE(Users, m_users);
	ADD_JSON_VALUE(IsOpen, m_isOpen);
	ADD_JSON_VALUE(Name, m_name);
	ADD_JSON_VALUE(Description, m_description);
	ADD_JSON_VALUE(Language, m_languages);
	return object;
}

QStringList PublicGroupChat::splitLanguages(const QString &languages)
{
	QStringList result = splitString(languages.toLower(), QStringLiteral("|;, "));
	result.sort(Qt::CaseInsensitive);
	return result;
}

PublicGroupChats PublicGroupChat::fromJson(const QJsonArray &array)
{
	return transform<PublicGroupChats>(
		array, [](const auto &value) { return PublicGroupChat {value.toObject()}; });
}

QJsonArray PublicGroupChat::toJson(const PublicGroupChats &groupChats)
{
	return transformNoReserve<QJsonArray>(
		groupChats, [](const auto &groupChat) { return groupChat.toJson(); });
}
