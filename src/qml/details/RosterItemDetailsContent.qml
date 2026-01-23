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

	property ChatController chatController

	account: chatController.account
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
		currentIndex: automaticMediaDownloadsDelegate.indexOf(root.chatController.rosterItem.automaticMediaDownloadsRule)
		onActivated: root.chatController.account.rosterController.setAutomaticMediaDownloadsRule(root.chatController.jid, automaticMediaDownloadsDelegate.currentValue)
	}
	vCardArea.visible: vCardRepeater.count
	vCardExpansionButton {
		visible: vCardRepeater.count > 3
		checked: !visible
	}
	rosterGoupListView {
		header: Loader {
			sourceComponent: root.chatController.account.settings.enabled ? rosterGroupComponent : null
			width: ListView.view.width

			Component {
				id: rosterGroupComponent

				FormCard.FormCard {
					Kirigami.Theme.colorSet: Kirigami.Theme.Window

					FormCard.AbstractFormDelegate {
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
									let groups = root.chatController.rosterItem.groups

									if (groups.includes(rosterGroupField.text)) {
										rosterGroupField.clear()
									} else if (enabled) {
										rosterGroupBusyIndicator.visible = true

										groups.push(rosterGroupField.text)
										root.chatController.account.rosterController.updateGroups(root.chatController.jid, root.chatController.rosterItem.name, groups)

										rosterGroupField.clear()
									}

									rosterGroupField.forceActiveFocus()
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
								target: root.chatController.account.rosterController

								function onGroupsChanged() {
									rosterGroupBusyIndicator.visible = false
									rosterGroupField.forceActiveFocus()
								}
							}
						}
					}
				}
			}
		}
		delegate: FormCard.FormSwitchDelegate {
			id: rosterGroupDelegate
			text: modelData
			enabled: root.chatController.account.settings.enabled
			checked: root.chatController.rosterItem.groups.includes(modelData)
			width: ListView.view.width
			onClicked: {
				let groups = root.chatController.rosterItem.groups

				if (checked) {
					groups.push(modelData)
				} else {
					groups.splice(groups.indexOf(modelData), 1)
				}

				root.chatController.account.rosterController.updateGroups(root.chatController.jid, root.chatController.rosterItem.name, groups)
			}

			Connections {
				target: root.chatController

				// TODO: Remove the following once fixed in Kirigami Addons.
				function onRosterItemChanged() {
					// Update the "checked" value of "rosterGroupDelegate" as a workaround because
					// "FormCard.FormSwitchDelegate" does not listen to changes of
					// "root.chatController.rosterItem.groups".
					rosterGroupDelegate.checked = root.chatController.rosterItem.groups.includes(modelData)
				}
			}
		}
	}
}
