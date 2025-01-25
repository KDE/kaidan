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

	property RosterFilterProxyModel rosterFilterProxyModel

	spacing: 0

	InlineListView {
		id: typeListView
		model: [
			{
				display: qsTr("Unavailable contacts"),
				value: RosterFilterProxyModel.Type.UnavailableContact
			},
			{
				display: qsTr("Available contacts"),
				value: RosterFilterProxyModel.Type.AvailableContact
			},
			{
				display: qsTr("Private groups"),
				value: RosterFilterProxyModel.Type.PrivateGroupChat
			},
			{
				display: qsTr("Public groups"),
				value: RosterFilterProxyModel.Type.PublicGroupChat
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
				checked: root.rosterFilterProxyModel.displayedTypes
				onToggled: root.rosterFilterProxyModel.resetDisplayedTypes()

				// TODO: Remove this once fixed in Kirigami Addons.
				// Add a connection as a workaround to reset the switch because
				// "FormCard.FormSwitchDelegate" does not listen to changes of
				// "root.rosterFilterProxyModel".
				Connections {
					target: root.rosterFilterProxyModel

					function onDisplayedTypesChanged() {
						typeFilteringSwitch.checked = root.rosterFilterProxyModel.displayedTypes
					}
				}
			}
		}
		delegate: FormCard.FormSwitchDelegate {
			id: typeDelegate
			text: modelData.display
			checked: root.rosterFilterProxyModel.displayedTypes & modelData.value
			width: ListView.view.width
			onToggled: {
				if (checked) {
					root.rosterFilterProxyModel.addDisplayedType(modelData.value)
				} else {
					root.rosterFilterProxyModel.removeDisplayedType(modelData.value)
				}
			}

			// TODO: Remove this once fixed in Kirigami Addons.
			// Add a connection as a workaround to reset the switch because
			// "FormCard.FormSwitchDelegate" does not listen to changes of
			// "root.rosterFilterProxyModel".
			Connections {
				target: root.rosterFilterProxyModel

				function onDisplayedTypesChanged() {
					typeDelegate.checked = root.rosterFilterProxyModel.displayedTypes & modelData.value
				}
			}
		}
	}

	HorizontalSeparator {
		visible: !Kirigami.Settings.isMobile && accountListView.visible
	}

	InlineListView {
		id: accountListView
		model: RosterModel.accountJids
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
				checked: root.rosterFilterProxyModel.selectedAccountJids.length
				onToggled: root.rosterFilterProxyModel.selectedAccountJids = []

				// TODO: Remove this once fixed in Kirigami Addons.
				// Add a connection as a workaround to reset the switch because
				// "FormCard.FormSwitchDelegate" does not listen to changes of
				// "root.rosterFilterProxyModel".
				Connections {
					target: root.rosterFilterProxyModel

					function onSelectedAccountJidsChanged() {
						accountFilteringSwitch.checked = root.rosterFilterProxyModel.selectedAccountJids.length
					}
				}
			}
		}
		delegate: FormCard.FormSwitchDelegate {
			id: accountDelegate
			text: modelData
			checked: root.rosterFilterProxyModel.selectedAccountJids.includes(modelData)
			width: ListView.view.width
			onToggled: {
				if(checked) {
					root.rosterFilterProxyModel.selectedAccountJids.push(modelData)
				} else {
					root.rosterFilterProxyModel.selectedAccountJids.splice(root.rosterFilterProxyModel.selectedAccountJids.indexOf(modelData), 1)
				}
			}

			// TODO: Remove this once fixed in Kirigami Addons.
			// Add a connection as a workaround to reset the switch because
			// "FormCard.FormSwitchDelegate" does not listen to changes of
			// "root.rosterFilterProxyModel".
			Connections {
				target: root.rosterFilterProxyModel

				function onSelectedAccountJidsChanged() {
					accountDelegate.checked = root.rosterFilterProxyModel.selectedAccountJids.includes(modelData)
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
				checked: root.rosterFilterProxyModel.selectedGroups.length
				onToggled: root.rosterFilterProxyModel.selectedGroups = []

				// TODO: Remove this once fixed in Kirigami Addons.
				// Add a connection as a workaround to reset the switch because
				// "FormCard.FormSwitchDelegate" does not listen to changes of
				// "root.rosterFilterProxyModel".
				Connections {
					target: root.rosterFilterProxyModel

					function onSelectedGroupsChanged() {
						groupFilteringSwitch.checked = root.rosterFilterProxyModel.selectedGroups.length
					}
				}
			}
		}
		delegate: FormCard.FormSwitchDelegate {
			id: groupDelegate
			text: modelData
			checked: root.rosterFilterProxyModel.selectedGroups.includes(modelData)
			width: ListView.view.width
			onToggled: {
				if (checked) {
					root.rosterFilterProxyModel.selectedGroups.push(modelData)
				} else {
					root.rosterFilterProxyModel.selectedGroups.splice(root.rosterFilterProxyModel.selectedGroups.indexOf(modelData), 1)
				}
			}

			// TODO: Remove this once fixed in Kirigami Addons.
			// Add a connection as a workaround to reset the switch because
			// "FormCard.FormSwitchDelegate" does not listen to changes of
			// "root.rosterFilterProxyModel".
			Connections {
				target: root.rosterFilterProxyModel

				function onSelectedGroupsChanged() {
					groupDelegate.checked = root.rosterFilterProxyModel.selectedGroups.includes(modelData)
				}
			}
		}
	}
}
