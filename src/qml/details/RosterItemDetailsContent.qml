// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls
import org.kde.kirigami 2.19 as Kirigami
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

import im.kaidan.kaidan 1.0

import "../elements"

DetailsContent {
	id: root

	property alias rosterItemWatcher: rosterItemWatcher

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
		currentIndex: automaticMediaDownloadsDelegate.indexOf(rosterItemWatcher.item.automaticMediaDownloadsRule)
		onActivated: RosterModel.setAutomaticMediaDownloadsRule(root.accountJid, root.jid, automaticMediaDownloadsDelegate.currentValue)
	}
	vCardArea.visible: vCardRepeater.count
	rosterGoupListView {
		header: MobileForm.FormCard {
			width: ListView.view.width
			Kirigami.Theme.colorSet: Kirigami.Theme.Window
			contentItem: MobileForm.AbstractFormDelegate {
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
							let groups = rosterItemWatcher.item.groups

							if (groups.includes(rosterGroupField.text)) {
								rosterGroupField.clear()
							} else if (enabled) {
								rosterGroupBusyIndicator.visible = true

								groups.push(rosterGroupField.text)
								Kaidan.client.rosterManager.updateGroupsRequested(root.jid, rosterItemWatcher.item.name, groups)

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
		delegate: MobileForm.FormSwitchDelegate {
			id: rosterGroupDelegate
			text: modelData
			checked: rosterItemWatcher.item.groups.includes(modelData)
			width: ListView.view.width
			onToggled: {
				let groups = rosterItemWatcher.item.groups

				if (checked) {
					groups.push(modelData)
				} else {
					groups.splice(groups.indexOf(modelData), 1)
				}

				Kaidan.client.rosterManager.updateGroupsRequested(root.jid, rosterItemWatcher.item.name, groups)
			}

			// TODO: Remove this and see TODO in RosterModel once fixed in Kirigami Addons.
			Connections {
				target: RosterModel

				function onGroupsChanged() {
					// Update the "checked" value of "rosterGroupDelegate" as a work
					// around because "MobileForm.FormSwitchDelegate" does not listen to
					// changes of "rosterItemWatcher.item.groups".
					rosterGroupDelegate.checked = rosterItemWatcher.item.groups.includes(modelData)
				}
			}
		}
	}

	// This needs to be placed after the notification section.
	// Otherwise, notifications will not be shown as muted after switching chats.
	// It is probably a bug in Kirigami Addons.
	RosterItemWatcher {
		id: rosterItemWatcher
		jid: root.jid
	}
}
