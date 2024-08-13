// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>

class Account
{
	Q_GADGET

public:
	/**
	 * Default rule to automatically download media for all roster items of an account.
	 */
	enum class AutomaticMediaDownloadsRule {
		Never,        ///< Never automatically download files.
		PresenceOnly, ///< Automatically download files only for contacts receiving presence.
		Always,       ///< Always automatically download files.
		Default = PresenceOnly,
	};
	Q_ENUM(AutomaticMediaDownloadsRule)

	Account() = default;

	bool operator==(const Account &other) const = default;
	bool operator!=(const Account &other) const = default;

	bool operator<(const Account &other) const;
	bool operator>(const Account &other) const;
	bool operator<=(const Account &other) const;
	bool operator>=(const Account &other) const;

	QString jid;
	QString name;
	QString latestMessageStanzaId;
	QDateTime latestMessageStanzaTimestamp;
	qint64 httpUploadLimit = 0;
};

Q_DECLARE_METATYPE(Account)
Q_DECLARE_METATYPE(Account::AutomaticMediaDownloadsRule)
