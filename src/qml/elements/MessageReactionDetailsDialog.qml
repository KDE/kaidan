// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

/**
 * This is a dialog listing the senders of emojis in reaction to a message.
 *
 * It provides information about the delivery state of own reactions and the functionality to resend
 * them in case of an error.
 */
Dialog {
	id: root

	property Account account
	property alias chatJid: messageReactionModel.chatJid
	property alias reactions: messageReactionModel.reactions

	topPadding: 0
	leftPadding: 0
	bottomPadding: 0
	rightPadding: 0
	header: null

	ListView {
		implicitHeight: contentHeight
		model: MessageReactionModel {
			id: messageReactionModel
			accountJid: root.account.settings.jid
		}
		delegate: ContactDelegate {
			id: messageReactionDelegate

			property var emojis: model.emojis

			jid: model.senderJid
			name: model.senderName
			width: ListView.view.width
			hoverEnabled: jid
			onClicked: {
				if (jid) {
					root.close()
					MainController.openChatPageRequested(root.account.settings.jid, jid)
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
		}
	}
}
