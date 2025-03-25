// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

import "../elements"

DetailsHeader {
	id: root
	jid: ChatController.chatJid

	property alias description: description

	displayName: ChatController.rosterItem.displayName
	avatarAction: Kirigami.Action {
		text: qsTr("Maximize avatar")
		icon.name: "view-fullscreen-symbolic"
		enabled: Kaidan.avatarStorage.getAvatarUrl(ChatController.chatJid).toString()
		onTriggered: Qt.openUrlExternally(Kaidan.avatarStorage.getAvatarUrl(ChatController.chatJid))
	}

	Controls.Label {
		id: description
		color: Kirigami.Theme.neutralTextColor
		wrapMode: Text.Wrap
		Layout.fillWidth: true
	}

	function changeDisplayName(newDisplayName) {
		Kaidan.rosterController.renameContact(ChatController.chatJid, newDisplayName)
	}

	function handleDisplayNameChanged() {}
}
