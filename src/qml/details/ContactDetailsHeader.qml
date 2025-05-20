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
	avatarAction: Kirigami.Action {
		text: qsTr("Maximize avatar")
		icon.name: "view-fullscreen-symbolic"
		enabled: root.chatController.account.avatarCache.getAvatarUrl(root.chatController.jid).toString()
		onTriggered: Qt.openUrlExternally(root.chatController.account.avatarCache.getAvatarUrl(root.chatController.jid))
	}
}
