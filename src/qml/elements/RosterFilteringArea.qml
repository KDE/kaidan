// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiraghav@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

import im.kaidan.kaidan 1.0

/**
 * Used to filter the displayed roster items.
 */
ColumnLayout {
	id: root

	property RosterFilterProxyModel rosterFilterProxyModel

	spacing: 0

	MobileForm.FormCard {
		implicitWidth: 570
		Layout.fillWidth: true
		Kirigami.Theme.colorSet: Kirigami.Theme.Window
		contentItem: MobileForm.FormSwitchDelegate {
			id: chatFilteringSwitch
			text: qsTr("Filter by availability")
			description: qsTr("Show only available contacts")
			checked: root.rosterFilterProxyModel.onlyAvailableContactsShown
			onToggled: root.rosterFilterProxyModel.onlyAvailableContactsShown = checked
		}
	}

	ListView {
		id: accountListView
		model: RosterModel.accountJids
		visible: count > 1
		implicitWidth: 570
		implicitHeight: contentHeight
		Layout.fillWidth: true
		header: MobileForm.FormCard {
			width: ListView.view.width
			Kirigami.Theme.colorSet: Kirigami.Theme.Window
			contentItem: MobileForm.FormSwitchDelegate {
				id: accountFilteringSwitch
				text: qsTr("Filter by accounts")
				description: qsTr("Show only chats of selected accounts")
				enabled: checked
				checked: root.rosterFilterProxyModel.selectedAccountJids.length
				onToggled: root.rosterFilterProxyModel.selectedAccountJids = []

				// TODO: Remove this once fixed in Kirigami Addons.
				// Add a connection as a work around to reset the switch because
				// "MobileForm.FormSwitchDelegate" does not listen to changes of
				// "root.rosterFilterProxyModel".
				Connections {
					target: root.rosterFilterProxyModel

					function onSelectedAccountJidsChanged() {
						accountFilteringSwitch.checked = root.rosterFilterProxyModel.selectedAccountJids.length
					}
				}
			}
		}
		delegate: MobileForm.FormSwitchDelegate {
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
			// Add a connection as a work around to reset the switch because
			// "MobileForm.FormSwitchDelegate" does not listen to changes of
			// "root.rosterFilterProxyModel".
			Connections {
				target: root.rosterFilterProxyModel

				function onSelectedAccountJidsChanged() {
					accountDelegate.checked = root.rosterFilterProxyModel.selectedAccountJids.includes(modelData)
				}
			}
		}

		Connections {
			target: RosterModel

			function onAccountJidsChanged() {
				// Remove selected account JIDs that have been removed from the main model.
				const selectedAccountJids = root.rosterFilterProxyModel.selectedAccountJids
				for (let i = 0; i < selectedAccountJids.length; i++) {
					if (!RosterModel.accountJids.includes(selectedAccountJids[i])) {
						root.rosterFilterProxyModel.selectedAccountJids.splice(i, 1)
					}
				}
			}
		}
	}

	Kirigami.Separator {
		visible: !Kirigami.Settings.isMobile
		implicitHeight: Kirigami.Units.smallSpacing
		Layout.fillWidth: true
	}

	ListView {
		id: groupListView
		model: RosterModel.groups
		visible: count
		implicitWidth: 570
		implicitHeight: contentHeight
		Layout.fillWidth: true
		header: MobileForm.FormCard {
			width: ListView.view.width
			Kirigami.Theme.colorSet: Kirigami.Theme.Window
			contentItem: MobileForm.FormSwitchDelegate {
				id: groupFilteringSwitch
				text: qsTr("Filter by labels")
				description: qsTr("Show only chats with selected labels")
				enabled: checked
				checked: root.rosterFilterProxyModel.selectedGroups.length
				onToggled: root.rosterFilterProxyModel.selectedGroups = []

				// TODO: Remove this once fixed in Kirigami Addons.
				// Add a connection as a work around to reset the switch because
				// "MobileForm.FormSwitchDelegate" does not listen to changes of
				// "root.rosterFilterProxyModel".
				Connections {
					target: root.rosterFilterProxyModel

					function onSelectedGroupsChanged() {
						groupFilteringSwitch.checked = root.rosterFilterProxyModel.selectedGroups.length
					}
				}
			}
		}
		delegate: MobileForm.FormSwitchDelegate {
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
			// Add a connection as a work around to reset the switch because
			// "MobileForm.FormSwitchDelegate" does not listen to changes of
			// "root.rosterFilterProxyModel".
			Connections {
				target: root.rosterFilterProxyModel

				function onSelectedGroupsChanged() {
					groupDelegate.checked = root.rosterFilterProxyModel.selectedGroups.includes(modelData)
				}
			}
		}

		Connections {
			target: RosterModel

			function onGroupsChanged() {
				// Remove selected groups that have been removed from the main model.
				const selectedGroups = root.rosterFilterProxyModel.selectedGroups
				for (let i = 0; i < selectedGroups.length; i++) {
					const selectedGroup = selectedGroups[i]
					if (!RosterModel.groups.includes(selectedGroups[i])) {
						root.rosterFilterProxyModel.selectedGroups.splice(i, 1)
					}
				}
			}
		}
	}
}
