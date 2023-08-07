// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

/**
 * This is a sheet listing the senders of emojis in reaction to a message.
 * It provides information about the delivery state of own reactions and the functionality to resend
 * them in case of an error.
 */
Kirigami.OverlaySheet {
	id: root

	property string messageId
	property bool isOwnMessage
	property var detailedReactions
	property var ownDetailedReactions

	showCloseButton: false
	header: Kirigami.Heading {
		text: qsTr("Reactions")
	}

	footer: Loader {
		// Using an empty item is necessary because the sheet needs an item as its footer once the
		// footer has been initialized and does not provide a way to reset the footer.
		// If "undefined" instead of "emptyItemComponent" was used, the footer's height would be
		// more than needed.
		// If the footer was set to "null", an error would occur when the sheet tries to access the
		// footer.
		sourceComponent: root.ownDetailedReactions && root.ownDetailedReactions.length ? ownDetailedReactionsAreaComponent : emptyItemComponent

		Component {
			id: emptyItemComponent

			Item {}
		}

		Component {
			id: ownDetailedReactionsAreaComponent

			RowLayout {
				Kirigami.Heading {
					text: qsTr("Own:")
				}

				Flow {
					spacing: 4
					Layout.fillWidth: true

					Repeater {
						model: root.ownDetailedReactions

						MessageReactionDisplayButton {
							accentColor: secondaryBackgroundColor
							deliveryState: modelData.deliveryState
							isOwnMessage: root.isOwnMessage
							text: modelData.emoji
							onClicked: {
								const resendMessageReactions = function () {
									MessageModel.resendMessageReactions(root.messageId)
								}

								if (deliveryState === MessageReactionDeliveryState.PendingAddition) {
									passiveNotification(qsTr("%1 will be added once you are connected").arg(modelData.emoji))
								} else if (deliveryState === MessageReactionDeliveryState.PendingRemovalAfterSent ||
									deliveryState === MessageReactionDeliveryState.PendingRemovalAfterDelivered) {
									passiveNotification(qsTr("%1 will be removed once you are connected").arg(modelData.emoji))
								} else if (deliveryState === MessageReactionDeliveryState.ErrorOnAddition) {
									showPassiveNotification(qsTr("%1 could not be added").arg(modelData.emoji), "long", "Retry", resendMessageReactions)
								} else if (deliveryState === MessageReactionDeliveryState.ErrorOnRemovalAfterSent ||
									deliveryState === MessageReactionDeliveryState.ErrorOnRemovalAfterDelivered) {
									showPassiveNotification(qsTr("%1 could not be removed").arg(modelData.emoji), "long", "Retry", resendMessageReactions)
								} else if (deliveryState === MessageReactionDeliveryState.Sent) {
									showPassiveNotification(qsTr("%1 has been sent").arg(modelData.emoji))
								} else if (deliveryState === MessageReactionDeliveryState.Delivered) {
									showPassiveNotification(qsTr("%1 has been delivered").arg(modelData.emoji))
								}
							}
						}
					}
				}
			}
		}
	}

	ListView {
		model: detailedReactions
		implicitWidth: largeButtonWidth
		delegate: UserListItem {
			id: sender
			accountJid: AccountManager.jid
			jid: modelData.senderJid
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

			// right: emojis
			RowLayout {
				Layout.fillWidth: true

				Item {
					Layout.fillWidth: true
				}

				Flow {
					spacing: 4

					Repeater {
						model: modelData.emojis

						Kirigami.Heading {
							text: modelData
						}
					}
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
