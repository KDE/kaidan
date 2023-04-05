// SPDX-FileCopyrightText: 2023 Mathis Brüchert <mbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

import "../elements"

ColumnLayout {
	id: root

	default property alias __data: mainArea.data
	property Kirigami.OverlaySheet sheet
	required property string jid
	required property string displayName
	required property Kirigami.Action avatarAction

	RowLayout {
		Layout.topMargin: Kirigami.Units.largeSpacing * 2
		Layout.bottomMargin: Kirigami.Settings.isMobile ? 0 : Kirigami.Units.largeSpacing
		Layout.leftMargin: Kirigami.Settings.isMobile ? Layout.topMargin : Kirigami.Units.largeSpacing * 2
		Layout.rightMargin: Kirigami.Settings.isMobile ? Layout.leftMargin : 0

		Avatar {
			jid: root.jid
			name: root.displayName
			Layout.preferredHeight: Kirigami.Units.gridUnit * 8
			Layout.preferredWidth: Layout.preferredHeight

			// TODO: Make icon also visible when the cursor is on it directly after opening this page
			MouseArea {
				anchors.fill: parent
				hoverEnabled: true
				cursorShape: Qt.PointingHandCursor
				visible: avatarAction.enabled
				onEntered: {
					avatarActionHoverImage.visible = true
					avatarHoverFadeInAnimation.start()
				}
				onExited: avatarHoverFadeOutAnimation.start()
				onClicked: root.avatarAction.triggered()

				Kirigami.Icon {
					id: avatarActionHoverImage
					source: root.avatarAction.icon.name
					color: Kirigami.Theme.backgroundColor
					width: parent.width / 2
					height: width
					anchors.centerIn: parent
					opacity: 0
					visible: false

					NumberAnimation on opacity {
						id: avatarHoverFadeInAnimation
						from: 0
						to: 0.8
						duration: 250
					}

					NumberAnimation on opacity {
						id: avatarHoverFadeOutAnimation
						from: 0.8
						to: 0
						duration: 250
					}
				}
			}
		}

		ColumnLayout {
			id: mainArea
			Layout.leftMargin: 15

			RowLayout {
				id: displayNameArea
				spacing: 0

				Button {
					id: displayNameEditingIcon
					Controls.ToolTip.text: qsTr("Change name…")
					icon.name: "document-edit-symbolic"
					display: Controls.AbstractButton.IconOnly
					checked: !displayNameText.visible
					flat: true
					Layout.preferredWidth: Layout.preferredHeight
					Layout.preferredHeight: displayNameTextField.implicitHeight
					onHoveredChanged: {
						if (hovered) {
							flat = false
						} else {
							flat = true
						}
					}
					onClicked: {
						if (displayNameText.visible) {
							displayNameTextField.visible = true
							displayNameTextField.forceActiveFocus()
							displayNameTextField.selectAll()
						} else {
							displayNameTextField.visible = false
							displayNameArea.changeDisplayName(displayNameTextField.text)
						}
					}
				}

				Kirigami.Heading {
					id: displayNameText
					text: root.displayName
					textFormat: Text.PlainText
					maximumLineCount: 1
					elide: Text.ElideRight
					visible: !displayNameTextField.visible
					Layout.alignment: Qt.AlignVCenter
					Layout.fillWidth: true
					leftPadding: Kirigami.Units.largeSpacing
					// TODO: Get update of current vCard by using Entity Capabilities
					onTextChanged: displayNameChangedFunction()

					MouseArea {
						anchors.fill: displayNameText
						hoverEnabled: true
						cursorShape: Qt.PointingHandCursor
						onEntered: displayNameEditingIcon.flat = false
						onExited: displayNameEditingIcon.flat = true
						onClicked: displayNameEditingIcon.clicked(Qt.LeftButton)
					}
				}

				Controls.TextField {
					id: displayNameTextField
					text: displayNameText.text
					visible: false
					Layout.leftMargin: Kirigami.Units.largeSpacing
					Layout.fillWidth: true
					onAccepted: {
						displayNameArea.changeDisplayName(text)
						visible = false
					}
				}

				function changeDisplayName(newDisplayName) {
					if (newDisplayName !== root.displayName) {
						root.displayNameChangeFunction(newDisplayName)
					}
				}
			}

			Controls.Label {
				text: root.jid
				color: Kirigami.Theme.disabledTextColor
				textFormat: Text.PlainText
				maximumLineCount: 1
				elide: Text.ElideRight
				Layout.fillWidth: true
				Layout.leftMargin: 40
			}
		}
	}
}
