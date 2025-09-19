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
	avatar.isGroupChat: true
	description: root.chatController.rosterItem.isPublicGroupChat ? qsTr("Public Group Chat") : qsTr("Private Group Chat")

	Controls.Label {
		text: qsTr("Deleted")
		color: Kirigami.Theme.negativeTextColor
		visible: root.chatController.rosterItem.isDeletedGroupChat
		wrapMode: Text.Wrap
		Layout.fillWidth: true
	}
}
