// SPDX-FileCopyrightText: 2022 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QObject>

using PublicGroupChats = QVector<class PublicGroupChat>;

class PublicGroupChat
{
	Q_GADGET

	Q_PROPERTY(QString address READ address WRITE setAddress)
	Q_PROPERTY(int users READ users WRITE setUsers)
	Q_PROPERTY(bool isOpen READ isOpen WRITE setIsOpen)
	Q_PROPERTY(QString name READ name WRITE setName)
	Q_PROPERTY(QString description READ description WRITE setDescription)
	Q_PROPERTY(QStringList languages READ languages WRITE setLanguages)
	Q_PROPERTY(QJsonObject json READ toJson)

public:
	static constexpr const QStringView Address = u"address";
	static constexpr const QStringView Users = u"nusers";
	static constexpr const QStringView IsOpen = u"is_open";
	static constexpr const QStringView Name = u"name";
	static constexpr const QStringView Description = u"description";
	static constexpr const QStringView Language = u"language";

	explicit PublicGroupChat(const QJsonObject &object);
	PublicGroupChat(const PublicGroupChat &other);
	PublicGroupChat() = default;

	const QString &address() const;
	void setAddress(const QString &address);

	int users() const;
	void setUsers(int users);

	bool isOpen() const;
	void setIsOpen(bool isOpen);

	const QString &name() const;
	void setName(const QString &name);

	const QString &description() const;
	void setDescription(const QString &description);

	const QStringList &languages() const;
	void setLanguages(const QStringList &languages);

	PublicGroupChat &operator=(const PublicGroupChat &other);
	bool operator==(const PublicGroupChat &other) const = default;
	bool operator!=(const PublicGroupChat &other) const = default;

	QJsonObject toJson() const;

	static QStringList splitLanguages(const QString &languages);
	static PublicGroupChats fromJson(const QJsonArray &array);
	static QJsonArray toJson(const PublicGroupChats &groupChats);

private:
	QString m_address;
	int m_users;
	bool m_isOpen;
	QString m_name;
	QString m_description;
	QStringList m_languages;
};

Q_DECLARE_METATYPE(PublicGroupChat)
Q_DECLARE_METATYPE(PublicGroupChats)
