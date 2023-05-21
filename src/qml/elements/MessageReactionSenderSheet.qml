// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

/**
 * This is a sheet listing the senders of an emoji in reaction to a message for adding or removing
 * that emoji.
 */
Kirigami.OverlaySheet {
	id: root
	property string messageId
	property bool isOwnMessage
	property string emoji
	property var senderJids: []

	showCloseButton: false

	header: Kirigami.Heading {
		text: qsTr("Senders of %1").arg(root.emoji)
	}

	// TODO: Load / Set footer only if needed
	footer: RowLayout {
		CenteredAdaptiveButton {
			text: qsTr("Add own reaction")
			// TODO: Remove " && Kaidan.connectionState === Enums.StateConnected" once offline queue for message reactions is implemented
			visible: !root.senderJids.includes(AccountManager.jid) && !isOwnMessage && Kaidan.connectionState === Enums.StateConnected
			onClicked: {
				MessageModel.addMessageReaction(root.messageId, root.emoji)
				root.close()
			}
		}

		CenteredAdaptiveButton {
			text: qsTr("Remove own reaction")
			// TODO: Remove " && Kaidan.connectionState === Enums.StateConnected" once offline queue for message reactions is implemented
			visible: root.senderJids.includes(AccountManager.jid) && Kaidan.connectionState === Enums.StateConnected
			onClicked: {
				MessageModel.removeMessageReaction(root.messageId, root.emoji)
				root.close()
			}
		}
	}

	ListView {
		model: Object.keys(root.senderJids)
		implicitWidth: largeButtonWidth

		delegate: UserListItem {
			id: sender
			accountJid: AccountManager.jid
			jid: root.senderJids[modelData]
			name: senderWatcher.item.displayName

			RosterItemWatcher {
				id: senderWatcher
				jid: sender.jid
			}

			// middle
			ColumnLayout {
				spacing: Kirigami.Units.largeSpacing
				Layout.fillWidth: true

				// name
				Kirigami.Heading {
					text: name
					textFormat: Text.PlainText
					elide: Text.ElideRight
					maximumLineCount: 1
					level: 3
					Layout.fillWidth: true
					Layout.maximumHeight: Kirigami.Units.gridUnit * 1.5
				}
			}

			onClicked: {
				// Open the chatPage only if it is not yet open.
				if (jid != MessageModel.currentChatJid) {
					Kaidan.openChatPageRequested(accountJid, jid)
				}

				root.close()
			}
		}
	}
}
