// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick

import im.kaidan.kaidan

DetailsDialog {
	id: root

	property ChatController chatController

	account: chatController.account

	Connections {
		target: MainController

		// Close this dialog when the contact is removed.
		function onCloseChatPageRequested() {
			root.close()
		}
	}
}
