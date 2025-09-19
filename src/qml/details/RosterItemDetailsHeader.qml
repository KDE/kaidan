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

	property ChatController chatController
	property alias description: description.text

	account: chatController.account
	jid: chatController.jid
	displayName: chatController.rosterItem.displayName
	avatarAction: Kirigami.Action {
		text: qsTr("Maximize avatar")
		icon.name: "view-fullscreen-symbolic"
		enabled: root.avatar.source.toString()
		onTriggered: Qt.openUrlExternally(root.avatar.source)
	}

	Controls.Label {
		id: description
		color: Kirigami.Theme.neutralTextColor
		wrapMode: Text.Wrap
		Layout.fillWidth: true
	}

	function changeDisplayName(newDisplayName) {
		root.account.rosterController.renameContact(chatController.jid, newDisplayName)
	}

	function handleDisplayNameChanged() {}
}
