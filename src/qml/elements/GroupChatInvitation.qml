// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

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
