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

RosterItemDetailsContent {
	id: root

	property SimpleListViewSearchField inviteeSearchField
	property SimpleListViewSearchField userSearchField
	property SimpleListViewSearchField keyAuthenticationUserSearchField
	property int contactsBeingInvitedCount: 0

	topArea: [
		FormCard.FormCard {
			Layout.fillWidth: true

			FormCard.FormHeader {
				title: qsTr("Invite")
			}

			InlineListView {
				id: inviteeListView
				model: GroupChatInviteeFilterModel {
					sourceModel: RosterModel
					accountJid: root.chatController.account.settings.jid
					groupChatUserJids: root.chatController.groupChatUserJids
				}
				visible: inviteeExpansionButton.checked
				implicitHeight: contentHeight
				Layout.fillWidth: true
				header: FormCard.FormCard {
					width: ListView.view.width
					Kirigami.Theme.colorSet: Kirigami.Theme.Window

					FormCard.AbstractFormDelegate {
						background: null
						contentItem: RowLayout {
							spacing: Kirigami.Units.largeSpacing * 2

							SimpleListViewSearchField {
								id: inviteeSearchField
								listView: inviteeListView
								enabled: inviteeButton.visible
								Layout.fillWidth: true
								onVisibleChanged: {
									if (visible) {
										clear()
										forceActiveFocus()
									}
								}
								Component.onCompleted: root.inviteeSearchField = this
							}

							Button {
								id: inviteeButton
								Controls.ToolTip.text: qsTr("Invite selected contacts")
								icon.name: "go-next-symbolic"
								flat: !hovered
								Layout.preferredWidth: Layout.preferredHeight
								Layout.preferredHeight: inviteeSearchField.implicitHeight
								onClicked: {
									visible = false
									inviteeListView.inviteSelectedContacts()
									visible = true
								}
							}

							Controls.BusyIndicator {
								visible: !inviteeButton.visible
								Layout.preferredWidth: inviteeButton.Layout.preferredWidth
								Layout.preferredHeight: Layout.preferredWidth
								Layout.rightMargin: inviteeButton.Layout.rightMargin
							}
						}
					}
				}
				delegate: GroupChatInviteeDelegate {
					id: inviteeDelegate
					jid: model.jid
					name: model.name
					checked: model.selected
					width: ListView.view.width
					onClicked: {
						inviteeListView.model.sourceModel.toggleSelected(model.account.settings.jid, jid)
						root.inviteeSearchField.forceActiveFocus()
					}
					onActiveFocusChanged: {
						if (activeFocus) {
							root.inviteeSearchField.forceActiveFocus()
						}
					}
				}
				onVisibleChanged: {
					if (visible) {
						inviteeListView.model.sourceModel.resetSelected()
					}
				}

				function inviteSelectedContacts() {
					var groupChatPublic = root.chatController.rosterItem.groupChatFlags === RosterItem.GroupChatFlag.Public
					var sourceModel = inviteeListView.model.sourceModel

					for (var i = 0; i < sourceModel.rowCount(); i++) {
						if (sourceModel.data(sourceModel.index(i, 0), RosterModel.SelectedRole)) {
							var inviteeJid = sourceModel.data(sourceModel.index(i, 0), RosterModel.JidRole)
							root.chatController.account.groupChatController.inviteContactToGroupChat(root.chatController.jid, inviteeJid, groupChatPublic)
							root.contactsBeingInvitedCount++
						}
					}

					if (root.contactsBeingInvitedCount) {
						inviteeExpansionButton.toggle()
					} else {
						root.inviteeSearchField.forceActiveFocus()
						passiveNotification(qsTr("Select at least one contact"))
					}
				}
			}

			FormExpansionButton {
				id: inviteeExpansionButton
			}
		},

		FormCard.FormCard {
			Layout.fillWidth: true

			FormCard.FormHeader {
				title: qsTr("Participants")
			}

			FormCard.FormSectionText {
				text: qsTr("Allow a user (e.g., user@example.org) or all users of the same server (e.g., example.org) to participate")
				visible: userExpansionButton.checked
			}

			InlineListView {
				id: userListView
				model: GroupChatUserFilterModel {
					sourceModel: GroupChatUserModel {
						accountJid: root.chatController.account.settings.jid
						chatJid: root.chatController.jid
					}
				}
				visible: userExpansionButton.checked
				implicitHeight: contentHeight
				Layout.fillWidth: true
				header: FormCard.FormCard {
					width: ListView.view.width
					Kirigami.Theme.colorSet: Kirigami.Theme.Window

					FormCard.AbstractFormDelegate {
						background: null
						contentItem: RowLayout {
							Controls.Label {
								text: qsTr("You must be connected to change the participants")
								wrapMode: Text.Wrap
								visible: root.chatController.account.connection.state !== Enums.StateConnected
								Layout.fillWidth: true
							}

							SimpleListViewSearchField {
								listView: userListView
								visible: root.chatController.account.connection.state === Enums.StateConnected
								Layout.fillWidth: true
								onVisibleChanged: {
									if (visible) {
										clear()
										forceActiveFocus()
									}
								}
								Component.onCompleted: root.userSearchField = this
							}
						}
					}
				}
				section.property: "statusText"
				section.delegate: ListViewSectionDelegate {}
				delegate: GroupChatUserDelegate {
					account: root.chatController.account
					chatJid: root.chatController.jid
					jid: model.jid
					name: model.name
					width: ListView.view.width
					onActiveFocusChanged: {
						if (activeFocus) {
							root.userSearchField.forceActiveFocus()
						}
					}
				}
			}

			FormExpansionButton {
				id: userExpansionButton
			}
		}
	]
	mediaOverview {
		accountJid: root.chatController.account.settings.jid
		chatJid: root.chatController.jid
	}
	encryptionArea.delegates: [
		FormCard.FormHeader {
			title: qsTr("Encryption")
		},

		FormCard.FormSwitchDelegate {
			text: qsTr("OMEMO 2")
			description: qsTr("End-to-end encryption with OMEMO 2 ensures that nobody else than you and your chat partners can read or modify the data you exchange.")
			enabled: root.chatController.chatEncryptionWatcher.hasUsableDevices
			checked: enabled && root.chatController.encryption === Encryption.Omemo2
			// The switch is toggled by setting the user's preference on using encryption.
			// Note that 'checked' has already the value after the button is clicked.
			onClicked: root.chatController.encryption = checked ? Encryption.Omemo2 : Encryption.NoEncryption
		},

		AccountKeyAuthenticationButton {
			presenceCache: root.chatController.account.presenceCache
			jid: root.chatController.account.settings.jid
			encryptionWatcher: root.chatController.accountEncryptionWatcher
			onClicked: root.openKeyAuthenticationPage(groupChatDetailsAccountKeyAuthenticationPage)
		},

		GroupChatUserKeyAuthenticationButton {
			id: keyAuthenticationButton
			encryptionWatcher: root.chatController.chatEncryptionWatcher
			checkable: true
		},

		InlineListView {
			id: keyAuthenticationUserListView
			model: GroupChatUserKeyAuthenticationFilterModel {
				sourceModel: GroupChatUserModel {
					accountJid: root.chatController.account.settings.jid
					chatJid: root.chatController.jid
				}
			}
			visible: keyAuthenticationButton.visible && keyAuthenticationButton.checked
			implicitHeight: contentHeight
			Layout.fillWidth: true
			header: FormCard.FormCard {
				width: ListView.view.width
				Kirigami.Theme.colorSet: Kirigami.Theme.Window

				FormCard.AbstractFormDelegate {
					background: null
					contentItem: RowLayout {
						spacing: Kirigami.Units.largeSpacing * 3

						SimpleListViewSearchField {
							listView: keyAuthenticationUserListView
							Layout.fillWidth: true
							onVisibleChanged: {
								if (visible) {
									clear()
									forceActiveFocus()
								}
							}
							Component.onCompleted: root.keyAuthenticationUserSearchField = this
						}
					}
				}
			}
			delegate: ContactDelegate {
				id: keyAuthenticationUserDelegate
				jid: model.jid
				name: model.name
				width: ListView.view.width
				onActiveFocusChanged: {
					if (activeFocus) {
						root.keyAuthenticationUserSearchField.forceActiveFocus()
					}
				}
				onClicked: root.openKeyAuthenticationPage(groupChatDetailsKeyAuthenticationPage).jid = jid
			}
		}
	]
	qrCodeExpansionButton.description: qsTr("Share this group's chat address via QR code")
	qrCode: GroupChatQrCode {
		jid: root.chatController.jid
	}
	qrCodeButton {
		description: qsTr("Share this group's chat address via QR code")
		onClicked: qrCode.copyToClipboard()
	}
	uriButton {
		description: qsTr("Share this group's chat address via text")
		onClicked: {
			Utils.copyToClipboard(Utils.groupChatUriString(root.chatController.jid))
			passiveNotification(qsTr("Group address copied to clipboard"))
		}
	}
	invitationButton {
		description: qsTr("Share this group's chat address via a web page with usage help")
		onClicked: Utils.copyToClipboard(Utils.invitationUrlString(Utils.groupChatUriString(root.chatController.jid)))
	}

	FormCard.FormCard {
		Layout.fillWidth: true

		FormCard.FormHeader {
			title: qsTr("Notifications")
		}

		FormComboBoxDelegate {
			text: qsTr("Incoming messages")
			description: qsTr("Show notification and play sound on message arrival")
			model: [
				{
					display: qsTr("Account default"),
					value: RosterItem.NotificationRule.Account
				},
				{
					display: qsTr("Never"),
					value: RosterItem.NotificationRule.Never
				},
				{
					display: qsTr("On mention"),
					value: RosterItem.NotificationRule.Mentioned
				},
				{
					display: qsTr("Always"),
					value: RosterItem.NotificationRule.Always
				}
			]
			textRole: "display"
			valueRole: "value"
			currentIndex: indexOf(root.chatController.rosterItem.notificationRule)
			onActivated: root.chatController.account.rosterController.setNotificationRule(root.chatController.jid, currentValue)
		}
	}

	FormCard.FormCard {
		Layout.fillWidth: true

		FormCard.FormHeader {
			title: qsTr("Privacy")
		}

		FormCard.FormSwitchDelegate {
			text: qsTr("Send typing notifications")
			description: qsTr("Indicate when you have this conversation open, are typing and stopped typing")
			checked: root.chatController.rosterItem.chatStateSendingEnabled
			onToggled: root.chatController.account.rosterController.setChatStateSendingEnabled(root.chatController.jid, checked)
		}

		FormCard.FormSwitchDelegate {
			text: qsTr("Send read notifications")
			description: qsTr("Indicate which messages you have read")
			checked: root.chatController.rosterItem.readMarkerSendingEnabled
			onToggled: root.chatController.account.rosterController.setReadMarkerSendingEnabled(root.chatController.jid, checked)
		}
	}

	FormCard.FormCard {
		id: removalArea
		Layout.fillWidth: true
		visible: root.chatController.account.settings.enabled
		enabled: !groupChatLeavingButton.busy && !groupChatDeletionButton.busy

		FormCard.FormHeader {
			title: qsTr("Leaving & Deletion")
		}

		ConfirmationFormButtonArea {
			id: groupChatLeavingButton
			button {
				text: qsTr("Leave")
				description: qsTr("Leave group and remove complete chat history")
				icon.name: "edit-delete-symbolic"
				icon.color: Kirigami.Theme.neutralTextColor
			}
			confirmationButton.onClicked: {
				groupChatDeletionButton.confirmationButton.visible = false
				root.chatController.account.groupChatController.leaveGroupChat(root.chatController.jid)
			}
			busy: root.chatController.account.groupChatController.busy
			busyText: qsTr("Leaving group chat…")

			Connections {
				target: root.chatController.account.groupChatController

				function onGroupChatLeavingFailed(groupChatJid, errorMessage) {
					passiveNotification(qsTr("The group %1 could not be left%2").arg(groupChatJid).arg(errorMessage ? ": " + errorMessage : ""))
				}
			}
		}

		ConfirmationFormButtonArea {
			id: groupChatDeletionButton
			button {
				text: qsTr("Delete")
				description: qsTr("Delete group. Nobody will be able to join the group again!")
				icon.name: "edit-delete-symbolic"
				icon.color: Kirigami.Theme.negativeTextColor
			}
			confirmationButton.onClicked: {
				groupChatLeavingButton.confirmationButton.visible = false
				root.chatController.account.groupChatController.deleteGroupChat(root.chatController.jid)
			}
			busy: root.chatController.account.groupChatController.busy
			busyText: qsTr("Deleting group chat…")

			Connections {
				target: root.chatController.account.groupChatController

				function onGroupChatDeletionFailed(groupChatJid, errorMessage) {
					passiveNotification(qsTr("The group %1 could not be deleted%2").arg(groupChatJid).arg(errorMessage ? ": " + errorMessage : ""))
				}
			}
		}
	}

	function openInviteeListView() {
		inviteeExpansionButton.toggle()
	}

	function openKeyAuthenticationUserListView() {
		keyAuthenticationButton.toggle()
	}
}
