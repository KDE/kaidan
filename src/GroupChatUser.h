// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QString>

/**
 * This class represents a user of a group chat.
 */
struct GroupChatUser
{
	Q_GADGET

public:
	enum class Status {
		Allowed,
		Joined,
		Left,
		Banned,
	};
	Q_ENUM(Status)
	// TODO: Use QFlags here to allow combinations such as allowed but not joined

	QString displayName() const;

	bool operator==(const GroupChatUser &other) const;
	bool operator!=(const GroupChatUser &other) const;

	bool operator<(const GroupChatUser &other) const;

	QString accountJid;
	QString chatJid;
	// The ID is part of the primary key in the database.
	// Thus, it must not be NULL.
	// "" is set because a null string would otherwise be inserted as NULL into the database.
	QString id = QStringLiteral("");
	QString jid;
	QString name;
	Status status;
};

Q_DECLARE_METATYPE(GroupChatUser);
