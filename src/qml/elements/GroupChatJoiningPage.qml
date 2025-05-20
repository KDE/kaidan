// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
	id: root

	property alias account: content.account
	property alias groupChatJid: content.groupChatJid

	title: qsTr("Join group chat")
	Component.onCompleted: content.groupChatJidField.forceActiveFocus()

	GroupChatJoiningContent {
		id: content
	}
}
