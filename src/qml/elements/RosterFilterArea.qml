// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiraghav@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan

/**
 * Used to filter the displayed roster items.
 */
ColumnLayout {
	id: root

	property RosterFilterModel rosterFilterModel

	spacing: 0

	InlineListView {
		id: typeListView
		model: [
			{
				display: qsTr("Available contacts"),
				value: RosterFilterModel.Type.AvailableContact
			},
			{
				display: qsTr("Unavailable contacts"),
				value: RosterFilterModel.Type.UnavailableContact
			},
			{
				display: qsTr("Private groups"),
				value: RosterFilterModel.Type.PrivateGroupChat
			},
			{
				display: qsTr("Public groups"),
				value: RosterFilterModel.Type.PublicGroupChat
			}
		]
		implicitWidth: largeButtonWidth
		implicitHeight: contentHeight
		Layout.fillWidth: true
		header: FormCard.FormCard {
			width: ListView.view.width
			Kirigami.Theme.colorSet: Kirigami.Theme.Window

			FormCard.FormSwitchDelegate {
				id: typeFilteringSwitch
				text: qsTr("Filter by type")
				description: qsTr("Show only entries of specific types")
				enabled: checked
				checked: root.rosterFilterModel.displayedTypes
				onToggled: root.rosterFilterModel.resetDisplayedTypes()

				// TODO: Remove this once fixed in Kirigami Addons.
				// Add a connection as a workaround to reset the switch because
				// "FormCard.FormSwitchDelegate" does not listen to changes of
				// "root.rosterFilterModel".
				Connections {
					target: root.rosterFilterModel

					function onDisplayedTypesChanged() {
						typeFilteringSwitch.checked = root.rosterFilterModel.displayedTypes
					}
				}
			}
		}
		delegate: FormCard.FormSwitchDelegate {
			id: typeDelegate
			text: modelData.display
			checked: root.rosterFilterModel.displayedTypes & modelData.value
			width: ListView.view.width
			onToggled: {
				if (checked) {
					root.rosterFilterModel.addDisplayedType(modelData.value)
				} else {
					root.rosterFilterModel.removeDisplayedType(modelData.value)
				}
			}

			// TODO: Remove this once fixed in Kirigami Addons.
			// Add a connection as a workaround to reset the switch because
			// "FormCard.FormSwitchDelegate" does not listen to changes of
			// "root.rosterFilterModel".
			Connections {
				target: root.rosterFilterModel

				function onDisplayedTypesChanged() {
					typeDelegate.checked = root.rosterFilterModel.displayedTypes & modelData.value
				}
			}
		}
	}

	HorizontalSeparator {
		visible: !Kirigami.Settings.isMobile && accountListView.visible
	}

	InlineListView {
		id: accountListView
		model: AccountController.accounts
		visible: count > 1
		implicitWidth: 570
		implicitHeight: contentHeight
		Layout.fillWidth: true
		header: FormCard.FormCard {
			width: ListView.view.width
			Kirigami.Theme.colorSet: Kirigami.Theme.Window

			FormCard.FormSwitchDelegate {
				id: accountFilteringSwitch
				text: qsTr("Filter by accounts")
				description: qsTr("Show only chats of selected accounts")
				enabled: checked
				checked: root.rosterFilterModel.selectedAccountJids.length
				onToggled: root.rosterFilterModel.selectedAccountJids = []

				// TODO: Remove this once fixed in Kirigami Addons.
				// Add a connection as a workaround to reset the switch because
				// "FormCard.FormSwitchDelegate" does not listen to changes of
				// "root.rosterFilterModel".
				Connections {
					target: root.rosterFilterModel

					function onSelectedAccountJidsChanged() {
						accountFilteringSwitch.checked = root.rosterFilterModel.selectedAccountJids.length
					}
				}
			}
		}
		delegate: FormCard.FormSwitchDelegate {
			id: accountDelegate
			text: modelData.settings.displayName
			checked: root.rosterFilterModel.selectedAccountJids.includes(modelData.settings.jid)
			width: ListView.view.width
			onToggled: {
				if(checked) {
					root.rosterFilterModel.selectedAccountJids.push(modelData.settings.jid)
				} else {
					root.rosterFilterModel.selectedAccountJids.splice(root.rosterFilterModel.selectedAccountJids.indexOf(modelData.settings.jid), 1)
				}
			}

			// TODO: Remove this once fixed in Kirigami Addons.
			// Add a connection as a workaround to reset the switch because
			// "FormCard.FormSwitchDelegate" does not listen to changes of
			// "root.rosterFilterModel".
			Connections {
				target: root.rosterFilterModel

				function onSelectedAccountJidsChanged() {
					accountDelegate.checked = root.rosterFilterModel.selectedAccountJids.includes(modelData.settings.jid)
				}
			}
		}
	}

	HorizontalSeparator {
		visible: !Kirigami.Settings.isMobile && groupListView.visible
	}

	InlineListView {
		id: groupListView
		model: RosterModel.groups
		visible: count
		implicitWidth: 570
		implicitHeight: contentHeight
		Layout.fillWidth: true
		header: FormCard.FormCard {
			width: ListView.view.width
			Kirigami.Theme.colorSet: Kirigami.Theme.Window

			FormCard.FormSwitchDelegate {
				id: groupFilteringSwitch
				text: qsTr("Filter by labels")
				description: qsTr("Show only chats with selected labels")
				enabled: checked
				checked: root.rosterFilterModel.selectedGroups.length
				onToggled: root.rosterFilterModel.selectedGroups = []

				// TODO: Remove this once fixed in Kirigami Addons.
				// Add a connection as a workaround to reset the switch because
				// "FormCard.FormSwitchDelegate" does not listen to changes of
				// "root.rosterFilterModel".
				Connections {
					target: root.rosterFilterModel

					function onSelectedGroupsChanged() {
						groupFilteringSwitch.checked = root.rosterFilterModel.selectedGroups.length
					}
				}
			}
		}
		delegate: FormCard.FormSwitchDelegate {
			id: groupDelegate
			text: modelData
			checked: root.rosterFilterModel.selectedGroups.includes(modelData)
			width: ListView.view.width
			onToggled: {
				if (checked) {
					root.rosterFilterModel.selectedGroups.push(modelData)
				} else {
					root.rosterFilterModel.selectedGroups.splice(root.rosterFilterModel.selectedGroups.indexOf(modelData), 1)
				}
			}

			// TODO: Remove this once fixed in Kirigami Addons.
			// Add a connection as a workaround to reset the switch because
			// "FormCard.FormSwitchDelegate" does not listen to changes of
			// "root.rosterFilterModel".
			Connections {
				target: root.rosterFilterModel

				function onSelectedGroupsChanged() {
					groupDelegate.checked = root.rosterFilterModel.selectedGroups.includes(modelData)
				}
			}
		}
	}
}
