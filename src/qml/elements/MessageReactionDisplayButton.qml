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
 * This is a button for displaying an emoji in reaction to a message.
 */
MessageReactionButton {
	id: root

	default property alias __data: contentArea.data
	property string messageId
	property int count
	property int deliveryState
	property bool ownReactionIncluded: true
	property bool isOwnMessage

	primaryColor: {
		if (ownReactionIncluded) {
			if (deliveryState === MessageReactionDeliveryState.PendingAddition ||
				deliveryState === MessageReactionDeliveryState.PendingRemovalAfterSent ||
				deliveryState === MessageReactionDeliveryState.PendingRemovalAfterDelivered) {
				return Kirigami.Theme.neutralBackgroundColor
			} else if (deliveryState === MessageReactionDeliveryState.ErrorOnAddition ||
				deliveryState === MessageReactionDeliveryState.ErrorOnRemovalAfterSent ||
				deliveryState === MessageReactionDeliveryState.ErrorOnRemovalAfterDelivered) {
				return Kirigami.Theme.negativeBackgroundColor
			}

			return Kirigami.Theme.positiveBackgroundColor
		}

		return isOwnMessage ? primaryBackgroundColor : secondaryBackgroundColor
	}
	width: smallButtonWidth + (text.length < 3 ? 0 : (text.length - 2) * Kirigami.Theme.defaultFont.pixelSize * 0.6)
	contentItem: RowLayout {
		id: contentArea
		spacing: 0

		Controls.Label {
			text: root.text
			horizontalAlignment: Text.AlignHCenter
			font.family: "emoji"
			font.pixelSize: {
				const defaultFontSize = Kirigami.Theme.defaultFont.pixelSize

				if (counter.visible) {
					const characterCount = counter.text.length

					if (characterCount === 1) {
						return defaultFontSize * 1.5
					}

					if (characterCount === 2) {
						return defaultFontSize * 1.4
					}

					return defaultFontSize * 1.3
				}

				return defaultFontSize * 1.75
			}
			Layout.fillWidth: true
			// The emoji is not centered without the margin.
			// Other approaches such as "verticalAlignment: Text.AlignVCenter" and
			// "Layout.alignment: Qt.AlignCenter" do not work.
			Layout.topMargin: Kirigami.Theme.defaultFont.pixelSize * 0.1
		}

		Controls.Label {
			id: counter
			text: root.count > 99 ? "99+" : root.count
			visible: root.count > 1
			horizontalAlignment: Text.AlignLeft
			font.pixelSize: {
				const defaultFontSize = Kirigami.Theme.defaultFont.pixelSize
				const characterCount = text.length

				if (characterCount === 1) {
					return defaultFontSize * 1.1
				}

				if (characterCount === 2) {
					return defaultFontSize
				}

				return defaultFontSize * 0.9
			}
			Layout.fillWidth: true
		}
	}
	onClicked: {
		if (ownReactionIncluded &&
			deliveryState !== MessageReactionDeliveryState.PendingRemovalAfterSent &&
			deliveryState !== MessageReactionDeliveryState.PendingRemovalAfterDelivered) {
			MessageModel.removeMessageReaction(messageId, text)
		} else {
			MessageModel.addMessageReaction(messageId, text)
		}
	}
	Controls.ToolTip.text: {
		if (!ownReactionIncluded) {
			return ""
		}

		if (deliveryState === MessageReactionDeliveryState.PendingAddition) {
			return qsTr("Will be added once you are connected")
		}

		if (deliveryState === MessageReactionDeliveryState.PendingRemovalAfterSent ||
			deliveryState === MessageReactionDeliveryState.PendingRemovalAfterDelivered) {
			return qsTr("Will be removed once you are connected")
		}

		if (deliveryState === MessageReactionDeliveryState.ErrorOnAddition) {
			return qsTr("Could not be added")
		}

		if (deliveryState === MessageReactionDeliveryState.ErrorOnRemovalAfterSent ||
			deliveryState === MessageReactionDeliveryState.ErrorOnRemovalAfterDelivered) {
			return qsTr("Could not be removed")
		}

		if (deliveryState === MessageReactionDeliveryState.Sent) {
			return qsTr("Sent")
		}

		if (deliveryState === MessageReactionDeliveryState.Delivered) {
			return qsTr("Delivered")
		}
	}
}
