// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
	id: root

	property alias accountJid: content.accountJid
	property alias groupChatName: content.groupChatName
	property alias nickname: content.nickname

	title: qsTr("Create group chat")
	Component.onCompleted: content.groupChatNameField.forceActiveFocus()

	GroupChatCreationContent {
		id: content
	}
}
