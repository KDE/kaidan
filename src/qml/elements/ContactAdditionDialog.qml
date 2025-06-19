// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

Dialog {
	id: root

	property alias account: content.account
	property alias jid: content.jid
	property alias name: content.name

	title: qsTr("Add contact")
	bottomInset: 0
	padding: Kirigami.Units.mediumSpacing
	onOpened: content.jidField.forceActiveFocus()

	ContactAdditionContent {
		id: content
	}

	Connections {
		target: MainController

		function onOpenChatPageRequested(accountJid, chatJid) {
			root.close()
		}
	}
}
