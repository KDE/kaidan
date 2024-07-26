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
	QDateTime latestMessageTimestamp;
	qint64 httpUploadLimit = 0;
};

Q_DECLARE_METATYPE(Account)
