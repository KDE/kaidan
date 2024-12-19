// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan

import "../elements"

DetailsContent {
	id: root
	automaticMediaDownloadsDelegate {
		model: [
			{
				display: qsTr("Account default"),
				value: RosterItem.AutomaticMediaDownloadsRule.Account
			},
			{
				display: qsTr("Never"),
				value: RosterItem.AutomaticMediaDownloadsRule.Never
			},
			{
				display: qsTr("Always"),
				value: RosterItem.AutomaticMediaDownloadsRule.Always
			}
		]
		textRole: "display"
		valueRole: "value"
		currentIndex: automaticMediaDownloadsDelegate.indexOf(ChatController.rosterItem.automaticMediaDownloadsRule)
		onActivated: RosterModel.setAutomaticMediaDownloadsRule(ChatController.accountJid, ChatController.chatJid, automaticMediaDownloadsDelegate.currentValue)
	}
	vCardArea.visible: vCardRepeater.count
	rosterGoupListView {
		header: FormCard.FormCard {
			width: ListView.view.width
			Kirigami.Theme.colorSet: Kirigami.Theme.Window
			contentItem: FormCard.AbstractFormDelegate {
				background: null
				contentItem: RowLayout {
					spacing: Kirigami.Units.largeSpacing * 3

					Controls.TextField {
						id: rosterGroupField
						placeholderText: qsTr("New label")
						enabled: !rosterGroupBusyIndicator.visible
						Layout.fillWidth: true
						onAccepted: rosterGroupAdditionButton.clicked()
						onVisibleChanged: {
							if (visible) {
								clear()
								forceActiveFocus()
							}
						}
					}

					Button {
						id: rosterGroupAdditionButton
						Controls.ToolTip.text: qsTr("Add label")
						icon.name: "list-add-symbolic"
						enabled: rosterGroupField.text.length
						visible: !rosterGroupBusyIndicator.visible
						flat: !hovered
						Layout.preferredWidth: Layout.preferredHeight
						Layout.preferredHeight: rosterGroupField.implicitHeight
						Layout.rightMargin: Kirigami.Units.largeSpacing
						onClicked: {
							let groups = ChatController.rosterItem.groups

							if (groups.includes(rosterGroupField.text)) {
								rosterGroupField.clear()
							} else if (enabled) {
								rosterGroupBusyIndicator.visible = true

								groups.push(rosterGroupField.text)
								Kaidan.client.rosterManager.updateGroupsRequested(ChatController.chatJid, ChatController.rosterItem.name, groups)

								rosterGroupField.clear()
							} else {
								rosterGroupField.forceActiveFocus()
							}
						}
					}

					Controls.BusyIndicator {
						id: rosterGroupBusyIndicator
						visible: false
						Layout.preferredWidth: rosterGroupAdditionButton.Layout.preferredWidth
						Layout.preferredHeight: Layout.preferredWidth
						Layout.rightMargin: rosterGroupAdditionButton.Layout.rightMargin
					}

					Connections {
						target: RosterModel

						function onGroupsChanged() {
							rosterGroupBusyIndicator.visible = false
							rosterGroupField.forceActiveFocus()
						}
					}
				}
			}
		}
		delegate: FormCard.FormSwitchDelegate {
			id: rosterGroupDelegate
			text: modelData
			checked: ChatController.rosterItem.groups.includes(modelData)
			width: ListView.view.width
			onToggled: {
				let groups = ChatController.rosterItem.groups

				if (checked) {
					groups.push(modelData)
				} else {
					groups.splice(groups.indexOf(modelData), 1)
				}

				Kaidan.client.rosterManager.updateGroupsRequested(ChatController.chatJid, ChatController.rosterItem.name, groups)
			}

			Connections {
				target: ChatController

				// TODO: Remove the following once fixed in Kirigami Addons.
				function onRosterItemChanged() {
					// Update the "checked" value of "rosterGroupDelegate" as a workaround because
					// "FormCard.FormSwitchDelegate" does not listen to changes of
					// "ChatController.rosterItem.groups".
					rosterGroupDelegate.checked = ChatController.rosterItem.groups.includes(modelData)
				}
			}
		}
	}
}
