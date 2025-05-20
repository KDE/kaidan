// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick

import im.kaidan.kaidan

import "../elements"

FormInfoDialog {
	id: root

	property Account account

	DisabledAccountHint {
		account: root.account
	}

	Connections {
		target: MainController

		function onOpenChatPageRequested(accountJid, chatJid) {
			root.close()
		}
	}
}
