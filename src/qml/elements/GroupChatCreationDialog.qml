// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

Dialog {
	id: root

	property alias account: content.account

	title: qsTr("Create group chat")
	padding: Kirigami.Units.mediumSpacing
	onOpened: content.groupChatNameField.forceActiveFocus()

	GroupChatCreationContent {
		id: content
	}

	Connections {
		target: MainController

		function onOpenChatPageRequested(accountJid, chatJid) {
			root.close()
		}
	}
}
