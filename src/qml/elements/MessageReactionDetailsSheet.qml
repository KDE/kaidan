// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

/**
 * This is a sheet listing the senders of emojis in reaction to a message.
 * It provides information about the delivery state of own reactions and the functionality to resend
 * them in case of an error.
 */
Kirigami.Dialog {
	id: root

	property string accountJid
	property string chatJid
	property string messageId
	property alias reactions: messageReactionModel.reactions

	header: Item {}
	footer: Item {}

	ListView {
		implicitWidth: largeButtonWidth
		implicitHeight: contentHeight
		model: MessageReactionModel {
			id: messageReactionModel
			accountJid: root.accountJid
			chatJid: root.chatJid
			reactions: root.reactions
		}
		delegate: UserListItem {
			id: messageReactionDelegate

			property var emojis: model.emojis

			accountJid: root.accountJid
			jid: model.senderJid
			name: model.senderName
			width: ListView.view.width
			hoverEnabled: jid

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

			// right: emojis
			RowLayout {
				Layout.fillWidth: true

				Item {
					Layout.fillWidth: true
				}

				Flow {
					spacing: Kirigami.Units.largeSpacing

					Repeater {
						model: messageReactionDelegate.emojis

						Kirigami.Heading {
							text: modelData
							font.family: "emoji"
							font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.75
						}
					}
				}
			}
			onClicked: {
				if (jid) {
					Kaidan.openChatPageRequested(accountJid, jid)
					root.close()
				}
			}
		}
	}
}
