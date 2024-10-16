// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: 2021 Jan Blackquill <uhhadd@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

Item {
	id: backgroundRoot

	property QtObject message
	property color color: message.isOwn ? rightMessageBubbleColor : primaryBackgroundColor
	property int tailSize: Kirigami.Units.largeSpacing
	property bool showTail: true
	property alias dummy: dummy
	readonly property alias metaInfoWidth: metaInfo.width

	clip: true

	Item {
		id: tailBase
		clip: true
		visible: false

		anchors {
			top: parent.top
			bottom: parent.bottom
			left: parent.left
			leftMargin: -backgroundRoot.tailSize * 2
			rightMargin: -backgroundRoot.tailSize
			right: mainBG.left
		}
		Rectangle {
			color: backgroundRoot.color

			anchors.fill: parent
			anchors.bottomMargin: 4
			anchors.rightMargin: -backgroundRoot.tailSize
		}
	}
	Item {
		id: tailMask
		clip: true
		visible: false

		anchors {
			top: parent.top
			bottom: parent.bottom
			left: parent.left
			leftMargin: -backgroundRoot.tailSize * 2
			rightMargin: -backgroundRoot.tailSize
			right: mainBG.left
		}
		Kirigami.ShadowedRectangle {
			anchors.fill: parent
			anchors.rightMargin: backgroundRoot.tailSize

			width: backgroundRoot.tailSize * 3
			color: "black"

			corners {
				topLeftRadius: 0
				topRightRadius: backgroundRoot.tailSize * 10
				bottomRightRadius: 0
				bottomLeftRadius: 0
			}
		}
	}
	OpacityMask {
		visible: showTail
		anchors.fill: tailBase
		source: tailBase
		maskSource: tailMask
		invert: true
	}
	Rectangle {
		id: mainBG
		radius: roundedCornersRadius
		color: backgroundRoot.color
		anchors.fill: parent
		anchors.leftMargin: backgroundRoot.tailSize
	}

	RowLayout {
		id: metaInfo
		spacing: Kirigami.Units.smallSpacing
		anchors {
			bottom: parent.bottom
			right: mainBG.right
			margins: Kirigami.Units.smallSpacing
		}

		// warning for different encryption corner cases
		ScalableText {
			text: {
				if (backgroundRoot.message.encryption === Encryption.NoEncryption) {
					if (ChatController.isEncryptionEnabled) {
						// Encryption is set for the current chat but this message is unencrypted.
						return qsTr("Unencrypted")
					}
				} else if (ChatController.encryption !== Encryption.NoEncryption && backgroundRoot.message.trustLevel === Message.TrustLevel.Untrusted){
					// Encryption is set for the current chat but the key of this message's sender
					// is not trusted.
					return qsTr("Untrusted")
				}

				return ""
			}
			visible: text.length
			color: Kirigami.Theme.neutralTextColor
			font.italic: true
			scaleFactor: 0.9
		}

		ScalableText {
			text: backgroundRoot.message.errorText
			visible: text.length
			color: Kirigami.Theme.negativeTextColor
			scaleFactor: 0.9
		}

		ScalableText {
			text: backgroundRoot.message.time
			color: Kirigami.Theme.disabledTextColor
			opacity: 0.5
			scaleFactor: 0.9
		}

		Kirigami.Icon {
			source: backgroundRoot.message.encryption === Encryption.NoEncryption ? "channel-insecure-symbolic" : "channel-secure-symbolic"
			Layout.preferredWidth: Kirigami.Units.iconSizes.small
			Layout.preferredHeight: Layout.preferredWidth
		}

		Kirigami.Icon {
			source: {
				const trustLevel = backgroundRoot.message.trustLevel

				if (trustLevel === Message.TrustLevel.Authenticated) {
					return "security-high-symbolic"
				}

				if (trustLevel === Message.TrustLevel.Trusted) {
					return "security-medium-symbolic"
				}

				return "security-low-symbolic"
			}
			visible: backgroundRoot.message.encryption !== Encryption.NoEncryption
			Layout.preferredWidth: Kirigami.Units.iconSizes.small
			Layout.preferredHeight: Layout.preferredWidth
		}

		Kirigami.Icon {
			source: "document-edit-symbolic"
			visible: message.edited
			Layout.preferredWidth: Kirigami.Units.iconSizes.small
			Layout.preferredHeight: Layout.preferredWidth
		}

		Kirigami.Icon {
			source: message.deliveryStateIcon
			visible: message.isOwn && source
			Layout.preferredWidth: Kirigami.Units.iconSizes.small
			Layout.preferredHeight: Layout.preferredWidth

			MouseArea {
				id: checkmarkMouseArea
				anchors.fill: parent
				hoverEnabled: true
			}

			Controls.ToolTip {
				text: message.deliveryStateName
				visible: checkmarkMouseArea.containsMouse
				delay: 500
			}
		}
	}

	Controls.Label {
		id: dummy
		text: "⠀"
	}
}
