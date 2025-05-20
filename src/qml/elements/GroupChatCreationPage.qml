// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
	id: root

	property alias account: content.account

	title: qsTr("Create group chat")
	Component.onCompleted: content.groupChatNameField.forceActiveFocus()

	GroupChatCreationContent {
		id: content
	}
}
