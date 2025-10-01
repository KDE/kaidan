// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

ContactDelegate {
	id: root

	property string chatJid

	avatar.accountAvatar.visible: false

	Button {
		text: qsTr("Ban")
		icon.name: "edit-delete-symbolic"
		visible: root.account.settings.enabled && root.account.connection.state === Enums.StateConnected
		display: Controls.AbstractButton.IconOnly
		flat: !root.hovered
		Controls.ToolTip.text: text
		Layout.rightMargin: Kirigami.Units.smallSpacing * 3
		onClicked: root.account.groupChatController.banUser(root.account.settings.jid, root.chatJid, root.jid)
	}
}
