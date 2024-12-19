// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

ExtendedMessageContent {
	id: root

	property Item message

	mainArea.data: OpacityChangingMouseArea {
		opacityItem: parent.background
		acceptedButtons: Qt.LeftButton | Qt.RightButton
		onClicked: (event) => {
			if (event.button === Qt.LeftButton) {
				const chatJid = root.message.groupChatInvitationJid

				if (groupChatWatcher.item.isGroupChat) {
					Kaidan.openChatPageRequested(ChatController.accountJid, chatJid)
				} else {
					openView(groupChatJoiningDialog, groupChatJoiningPage).groupChatJid = chatJid
				}
			} else if (event.button === Qt.RightButton) {
				root.message.showContextMenu(this)
			}
		}
	}
	mainAreaBackground {
		Behavior on opacity {
			NumberAnimation {}
		}
	}
	contentArea.data: [
		Kirigami.Icon {
			id: icon
			source: "resource-group"
			Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
		},

		Controls.Label {
			text: root.message.messageBody
			wrapMode: Text.Wrap
			Layout.minimumWidth: root.minimumWidth - mainArea.leftPadding - icon.width - contentArea.spacing - mainArea.rightPadding
			Layout.maximumWidth: root.maximumWidth - mainArea.leftPadding - icon.width - contentArea.spacing - mainArea.rightPadding
		}
	]

	RosterItemWatcher {
		id: groupChatWatcher
		jid: root.message.groupChatInvitationJid
	}
}
