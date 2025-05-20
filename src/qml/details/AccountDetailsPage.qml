// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts

import im.kaidan.kaidan

DetailsPage {
	id: root
	title: qsTr("Account Details")
	Component.onCompleted: AccountController.setActiveAccount(account)
	Component.onDestruction: AccountController.resetActiveAccount()

	AccountDetailsHeader {
		account: root.account
	}

	AccountDetailsContent {
		account: root.account
		Layout.fillWidth: true
	}

	Connections {
		target: AccountController

		// Close this dialog when the account is removed.
		function onAccountRemoved(jid) {
			if (jid === root.account.settings.jid) {
				popLayer()
			}
		}
	}
}
