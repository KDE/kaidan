// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Layouts
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

ContactDelegate {
	id: root

	property Account account
	property string chatJid

	IconButton {
		text: qsTr("Ban")
		icon.source: "edit-delete-symbolic"
		visible: root.account.settings.enabled && root.account.connection.state === Enums.StateConnected
		flat: !root.hovered
		Layout.rightMargin: Kirigami.Units.smallSpacing * 3
		onClicked: root.account.groupChatController.banUser(root.account.settings.jid, root.chatJid, root.jid)
	}
}
