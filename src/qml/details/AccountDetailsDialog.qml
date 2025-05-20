// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts

import im.kaidan.kaidan

DetailsDialog {
	id: root
	title: qsTr("Account Details")
	Component.onCompleted: AccountController.setActiveAccount(account)
	Component.onDestruction: AccountController.resetActiveAccount()

	AccountDetailsHeader {
		dialog: root
		account: root.account
	}

	AccountDetailsContent {
		dialog: root
		account: root.account
		Layout.fillWidth: true
	}

	Connections {
		target: AccountController

		// Close this dialog when the account is removed.
		function onAccountRemoved(jid) {
			if (jid === root.account.settings.jid) {
				root.close()
			}
		}
	}
}
