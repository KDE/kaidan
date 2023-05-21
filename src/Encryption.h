// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

#include <QXmppGlobal.h>

class Encryption
{
	Q_GADGET

public:
	enum Enum {
		NoEncryption = QXmpp::NoEncryption,  ///< No encryption
		Omemo2 = QXmpp::Omemo2               ///< XEP-0384 OMEMO Encryption since version 0.8
	};
	Q_ENUM(Enum)
};
