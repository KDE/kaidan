// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Account.h"

bool Account::operator<(const Account &other) const
{
	return jid < other.jid;
}

bool Account::operator>(const Account &other) const
{
	return jid > other.jid;
}

bool Account::operator<=(const Account &other) const
{
	return jid <= other.jid;
}

bool Account::operator>=(const Account &other) const
{
	return jid >= other.jid;
}
