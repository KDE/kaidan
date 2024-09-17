// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import org.kde.kirigami 2.19 as Kirigami

Kirigami.ScrollablePage {
	id: root

	property alias accountJid: content.accountJid
	property alias groupChatJid: content.groupChatJid
	property alias nickname: content.nickname

	title: qsTr("Join group chat")
	Component.onCompleted: content.groupChatJidField.forceActiveFocus()

	GroupChatJoiningContent {
		id: content
	}
}
