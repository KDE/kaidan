// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Layouts 1.14

import im.kaidan.kaidan 1.0

DetailsSheet {
	id: root

	AccountDetailsHeader {
		sheet: root
		jid: AccountManager.jid
	}

	AccountDetailsContent {
		sheet: root
		jid: AccountManager.jid
		Layout.fillWidth: true
	}
}
