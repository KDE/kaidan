// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

import "../elements"

RosterItemDetailsHeader {
	id: root
	isGroupChat: true
	avatarAction: Kirigami.Action {
		text: qsTr("Maximize avatar")
		icon.name: "view-fullscreen-symbolic"
		enabled: Kaidan.avatarStorage.getAvatarUrl(ChatController.chatJid)
		onTriggered: Qt.openUrlExternally(Kaidan.avatarStorage.getAvatarUrl(ChatController.chatJid))
	}
	description.text: ChatController.rosterItem.isPublicGroupChat ? qsTr("Public Group Chat") : qsTr("Private Group Chat")

	Controls.Label {
		text: qsTr("Deleted")
		color: Kirigami.Theme.negativeTextColor
		visible: ChatController.rosterItem.isDeletedGroupChat
		wrapMode: Text.Wrap
		Layout.fillWidth: true
	}
}
