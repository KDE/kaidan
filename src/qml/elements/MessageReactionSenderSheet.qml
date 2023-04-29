/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2023 Kaidan developers and contributors
 *  (see the LICENSE file for a full list of copyright authors)
 *
 *  Kaidan is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  In addition, as a special exception, the author of Kaidan gives
 *  permission to link the code of its release with the OpenSSL
 *  project's "OpenSSL" library (or with modified versions of it that
 *  use the same license as the "OpenSSL" library), and distribute the
 *  linked executables. You must obey the GNU General Public License in
 *  all respects for all of the code used other than "OpenSSL". If you
 *  modify this file, you may extend this exception to your version of
 *  the file, but you are not obligated to do so.  If you do not wish to
 *  do so, delete this exception statement from your version.
 *
 *  Kaidan is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kaidan.  If not, see <http://www.gnu.org/licenses/>.
 */

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
