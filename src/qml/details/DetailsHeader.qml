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
	required property string jid
	required property string displayName
	property alias isGroupChat: avatar.isGroupChat
	required property Kirigami.Action avatarAction

	Avatar {
		id: avatar
		jid: root.jid
		name: root.displayName
		Layout.alignment: Qt.AlignHCenter
		Layout.preferredHeight: Kirigami.Units.gridUnit * 8
		Layout.preferredWidth: Layout.preferredHeight

		MouseArea {
			anchors.fill: parent
			visible: avatarAction.enabled
			hoverEnabled: true
			onHoveredChanged: {
				if (containsMouse) {
					avatarHoverFadeInAnimation.start()
				} else {
					avatarHoverFadeOutAnimation.start()
				}
			}
			onExited: avatarHoverFadeOutAnimation.start()
			onClicked: root.avatarAction.triggered()

			Rectangle {
				anchors.fill: parent
				color: Kirigami.Theme.backgroundColor
				opacity: avatar.source.toString() ? avatarActionHoverImage.opacity * 0.5 : 0
			}

			Kirigami.Icon {
				id: avatarActionHoverImage
				source: root.avatarAction.icon.name
				color: Kirigami.Theme.textColor
				width: parent.width / 2
				height: width
				anchors.centerIn: parent

				NumberAnimation on opacity {
					id: avatarHoverFadeOutAnimation
					from: avatar.source.toString() ? 1 : 0.8
					to: 0
					duration: 250
				}

				NumberAnimation on opacity {
					id: avatarHoverFadeInAnimation
					from: 0
					to: avatar.source.toString() ? 1 : 0.8
					duration: 250
				}
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

			Kirigami.Heading {
				id: displayNameText
				text: root.displayName
				textFormat: Text.PlainText
				maximumLineCount: 1
				elide: Text.ElideMiddle
				visible: !displayNameTextField.visible
				Layout.alignment: Qt.AlignVCenter
				Layout.fillWidth: true
				leftPadding: Kirigami.Units.largeSpacing
				// TODO: Get update of current vCard by using Entity Capabilities
				onTextChanged: handleDisplayNameChanged()

				MouseArea {
					id: displayNameMouseArea
					anchors.fill: displayNameText
					hoverEnabled: true
					onClicked: displayNameEditingButton.clicked()
				}
			}

			Controls.TextField {
				id: displayNameTextField
				text: displayNameText.text
				visible: false
				Layout.leftMargin: Kirigami.Units.largeSpacing
				Layout.fillWidth: true
				onAccepted: displayNameEditingButton.clicked()
			}
		}

		ColumnLayout {
			id: additionalInformationArea
			spacing: parent.spacing
			Layout.leftMargin: displayNameEditingButton.Layout.preferredWidth + displayNameTextField.Layout.leftMargin

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
