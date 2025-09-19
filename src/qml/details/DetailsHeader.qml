// SPDX-FileCopyrightText: 2023 Mathis Brüchert <mbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

import "../elements"

FormInfoHeader {
	id: root

	default property alias __data: additionalInformationArea.data
	property Account account
	required property string jid
	required property string displayName
	property alias displayNameEditable: displayNameEditingButton.visible
	property alias avatar: avatar
	required property Kirigami.Action avatarAction

	AccountRelatedAvatar {
		id: avatar
		jid: root.jid
		name: root.displayName
		accountAvatar {
			jid: root.account.settings.jid
			name: root.account.settings.displayName
		}
		implicitWidth: Kirigami.Units.gridUnit * 8
		implicitHeight: Kirigami.Units.gridUnit * 8
		accountAvatar.visible: root.account.settings.jid !== root.jid
		Layout.alignment: Qt.AlignHCenter

		MouseArea {
			id: avatarOverlay
			hoverEnabled: true
			anchors.top: avatar.top
			anchors.bottom: avatar.accountAvatar.bottom
			anchors.left: avatar.left
			anchors.right: avatar.accountAvatar.right
			visible: avatarAction.enabled
			onClicked: root.avatarAction.triggered()

			Rectangle {
				anchors.fill: parent
				color: Kirigami.Theme.backgroundColor
				opacity: avatarHoverIcon.opacity * 0.65
			}
		}

		Kirigami.Icon {
			id: avatarHoverIcon
			source: root.avatarAction.icon.name
			opacity: {
				if (avatarOverlay.containsMouse) {
					return avatar.source.toString() ? 1 : 0.8
				}

				return 0
			}
			width: avatar.implicitWidth / 2
			height: width
			anchors.centerIn: avatar

			Behavior on opacity {
				NumberAnimation {}
			}
		}
	}

	ColumnLayout {
		spacing: Kirigami.Units.largeSpacing
		Layout.leftMargin: root.flow === GridLayout.LeftToRight ? Kirigami.Units.largeSpacing * 2 : 0

		RowLayout {
			id: displayNameArea
			spacing: 0

			Button {
				id: displayNameEditingButton
				text: qsTr("Change name…")
				icon.name: "document-edit-symbolic"
				display: Controls.AbstractButton.IconOnly
				checkable: true
				checked: !displayNameText.visible
				flat: !hovered && !displayNameMouseArea.containsMouse
				visible: root.account.settings.enabled
				Controls.ToolTip.text: text
				Layout.preferredWidth: Layout.preferredHeight
				Layout.preferredHeight: displayNameTextField.implicitHeight
				onClicked: {
					if (displayNameText.visible) {
						displayNameTextField.visible = true
						displayNameTextField.forceActiveFocus()
						displayNameTextField.selectAll()
					} else {
						displayNameTextField.visible = false

						if (displayNameTextField.text !== root.displayName) {
							root.changeDisplayName(displayNameTextField.text)
						}
					}
				}
			}

			ScalableText {
				id: displayNameText
				text: root.displayName
				textFormat: Text.PlainText
				scaleFactor: 1.1
				maximumLineCount: 1
				elide: Text.ElideMiddle
				visible: !displayNameTextField.visible
				verticalAlignment: Text.AlignVCenter
				leftPadding: displayNameTextField.Layout.leftMargin + displayNameTextField.leftPadding
				Layout.preferredHeight: displayNameTextField.height
				Layout.fillWidth: true
				// TODO: Get update of current vCard by using Entity Capabilities
				onTextChanged: handleDisplayNameChanged()

				MouseArea {
					id: displayNameMouseArea
					anchors.fill: displayNameText
					hoverEnabled: true
					enabled: displayNameEditingButton.visible
					onClicked: displayNameEditingButton.clicked()
				}
			}

			Controls.TextField {
				id: displayNameTextField
				text: displayNameText.text
				font.pixelSize: displayNameText.font.pixelSize
				visible: false
				Layout.leftMargin: Kirigami.Units.smallSpacing
				Layout.fillWidth: true
				onAccepted: displayNameEditingButton.clicked()
			}
		}

		ColumnLayout {
			id: additionalInformationArea
			spacing: parent.spacing
			Layout.leftMargin: (displayNameEditingButton.visible ? displayNameEditingButton.Layout.preferredWidth : 0) + displayNameText.leftPadding

			Controls.Label {
				text: root.jid
				color: Kirigami.Theme.disabledTextColor
				textFormat: Text.PlainText
				maximumLineCount: 1
				elide: Text.ElideMiddle
				Layout.fillWidth: true
			}
		}
	}
}
