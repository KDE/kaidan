// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts

import im.kaidan.kaidan

import "../elements"

FormInfoDialog {
	id: root

	property string jid

	title: qsTr("Account Details")

	AccountDetailsHeader {
		dialog: root
		jid: root.jid
	}

	AccountDetailsContent {
		dialog: root
		jid: root.jid
		Layout.fillWidth: true
	}

	Connections {
		target: Kaidan

		// Close this dialog when the account is removed.
		function onCredentialsNeeded() {
			root.close()
		}

		// Close this dialog when the chat with oneself is added via it.
		function onOpenChatPageRequested(accountJid, chatJid) {
			root.close()
		}
	}
}
