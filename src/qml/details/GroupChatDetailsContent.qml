// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigami 2.19 as Kirigami
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

import im.kaidan.kaidan 1.0

import "../elements"

RosterItemDetailsContent {
	id: root

	property SimpleListViewSearchField contactSearchField
	property SimpleListViewSearchField userSearchField
	property SimpleListViewSearchField keyAuthenticationUserSearchField
	property int contactsBeingInvitedCount: 0

	topArea: [
		MobileForm.FormCard {
			Layout.fillWidth: true
			contentItem: ColumnLayout {
				spacing: 0

				MobileForm.FormCardHeader {
					title: qsTr("Invite")
				}

				ListView {
					id: contactListView
					model: RosterFilterProxyModel {
						sourceModel: RosterModel
						groupChatsExcluded: true
						groupChatUsersExcluded: true
					}
					visible: contactExpansionButton.checked
					implicitHeight: contentHeight
					Layout.fillWidth: true
					header: MobileForm.FormCard {
						width: ListView.view.width
						Kirigami.Theme.colorSet: Kirigami.Theme.Window
						contentItem: MobileForm.AbstractFormDelegate {
							background: null
							contentItem: RowLayout {
								spacing: Kirigami.Units.largeSpacing * 2

								SimpleListViewSearchField {
									id: contactSearchField
									listView: contactListView
									enabled: contactButton.visible
									Layout.fillWidth: true
									onVisibleChanged: {
										if (visible) {
											clear()
											forceActiveFocus()
										}
									}
									Component.onCompleted: root.contactSearchField = this
								}

								Button {
									id: contactButton
									Controls.ToolTip.text: qsTr("Invite selected contacts")
									icon.name: "go-next-symbolic"
									flat: !hovered
									Layout.preferredWidth: Layout.preferredHeight
									Layout.preferredHeight: contactSearchField.implicitHeight
									onClicked: {
										visible = false
										contactListView.inviteSelectedContacts()
										visible = true
									}
								}

								Controls.BusyIndicator {
									visible: !contactButton.visible
									Layout.preferredWidth: contactButton.Layout.preferredWidth
									Layout.preferredHeight: Layout.preferredWidth
									Layout.rightMargin: contactButton.Layout.rightMargin
								}
							}
						}
					}
					delegate: GroupChatContactInvitationItem {
						id: contactDelegate
						accountJid: model.accountJid
						jid: model.jid
						name: model.name
						selected: model.selected
						width: ListView.view.width
						onClicked: {
							contactListView.model.sourceModel.toggleSelected(model.accountJid, model.jid)
							root.contactSearchField.forceActiveFocus()
						}
						onActiveFocusChanged: {
							if (activeFocus) {
								root.contactSearchField.forceActiveFocus()
							}
						}
					}
					onVisibleChanged: {
						if (visible) {
							contactListView.model.sourceModel.resetSelected()
						}
					}

					function inviteSelectedContacts() {
						var groupChatPublic = ChatController.rosterItem.groupChatFlags === RosterItem.GroupChatFlag.Public
						var sourceModel = contactListView.model.sourceModel

						for (var i = 0; i < sourceModel.rowCount(); i++) {
							if (sourceModel.data(sourceModel.index(i, 0), RosterModel.SelectedRole)) {
								var inviteeJid = sourceModel.data(sourceModel.index(i, 0), RosterModel.JidRole)
								GroupChatController.inviteContactToGroupChat(ChatController.accountJid, ChatController.chatJid, inviteeJid, groupChatPublic)
								root.contactsBeingInvitedCount++
							}
						}

						if (root.contactsBeingInvitedCount) {
							contactExpansionButton.toggle()
						} else {
							root.contactSearchField.forceActiveFocus()
							passiveNotification(qsTr("Select at least one contact"))
						}
					}
				}

				FormExpansionButton {
					id: contactExpansionButton
				}
			}
		},

		MobileForm.FormCard {
			Layout.fillWidth: true
			contentItem: ColumnLayout {
				spacing: 0

				MobileForm.FormCardHeader {
					title: qsTr("Participants")
				}

				MobileForm.FormSectionText {
					text: qsTr("Allow a user (e.g., user@example.org) or all users of the same server (e.g., example.org) to participate")
					visible: userExpansionButton.checked
				}

				ListView {
					id: userListView
					model: GroupChatUserFilterModel {
						sourceModel: GroupChatUserModel {
							accountJid: ChatController.accountJid
							chatJid: ChatController.chatJid
						}
					}
					visible: userExpansionButton.checked
					clip: true
					implicitHeight: contentHeight
					Layout.fillWidth: true
					header: MobileForm.FormCard {
						width: ListView.view.width
						Kirigami.Theme.colorSet: Kirigami.Theme.Window
						contentItem: MobileForm.AbstractFormDelegate {
							background: null
							contentItem: RowLayout {
								Controls.Label {
									text: qsTr("You must be connected to change the participants")
									visible: Kaidan.connectionState !== Enums.StateConnected
									Layout.fillWidth: true
								}

								SimpleListViewSearchField {
									listView: userListView
									visible: Kaidan.connectionState === Enums.StateConnected
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
					delegate: GroupChatUserItem {
						id: userDelegate
						accountJid: ChatController.accountJid
						jid: model.jid
						name: model.name
						width: ListView.view.width
						onActiveFocusChanged: {
							if (activeFocus) {
								root.userSearchField.forceActiveFocus()
							}
						}

						Button {
							text: qsTr("Ban")
							icon.name: "edit-delete-symbolic"
							visible: Kaidan.connectionState === Enums.StateConnected
							display: Controls.AbstractButton.IconOnly
							flat: !userDelegate.hovered
							Controls.ToolTip.text: text
							Layout.rightMargin: Kirigami.Units.smallSpacing * 3
							onClicked: GroupChatController.banUser(ChatController.accountJid, ChatController.chatJid, userDelegate.jid)
						}
					}
				}

				FormExpansionButton {
					id: userExpansionButton
				}
			}
		}
	]
	mediaOverview {
		accountJid: ChatController.accountJid
		chatJid: ChatController.chatJid
	}
	encryptionArea: ColumnLayout {
		spacing: 0

		MobileForm.FormCardHeader {
			title: qsTr("Encryption")
		}

		MobileForm.FormSwitchDelegate {
			text: qsTr("OMEMO 2")
			description: qsTr("End-to-end encryption with OMEMO 2 ensures that nobody else than you and your chat partners can read or modify the data you exchange.")
			enabled: ChatController.chatEncryptionWatcher.hasUsableDevices
			checked: enabled && ChatController.encryption === Encryption.Omemo2
			// The switch is toggled by setting the user's preference on using encryption.
			// Note that 'checked' has already the value after the button is clicked.
			onClicked: ChatController.encryption = checked ? Encryption.Omemo2 : Encryption.NoEncryption
		}

		AccountKeyAuthenticationButton {
			jid: ChatController.accountJid
			encryptionWatcher: ChatController.accountEncryptionWatcher
			onClicked: root.openKeyAuthenticationPage(groupChatDetailsAccountKeyAuthenticationPage)
		}

		GroupChatUserKeyAuthenticationButton {
			id: keyAuthenticationButton
			encryptionWatcher: ChatController.chatEncryptionWatcher
			checkable: true
		}

		ListView {
			id: keyAuthenticationUserListView
			model: GroupChatUserKeyAuthenticationFilterModel {
				sourceModel: GroupChatUserModel {
					accountJid: ChatController.accountJid
					chatJid: ChatController.chatJid
				}
			}
			visible: keyAuthenticationButton.visible && keyAuthenticationButton.checked
			implicitHeight: contentHeight
			Layout.fillWidth: true
			header: MobileForm.FormCard {
				width: ListView.view.width
				Kirigami.Theme.colorSet: Kirigami.Theme.Window
				contentItem: MobileForm.AbstractFormDelegate {
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
			delegate: GroupChatUserItem {
				id: keyAuthenticationUserDelegate
				accountJid: ChatController.accountJid
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
	}
	qrCodeExpansionButton.description: qsTr("Share this group's chat address via QR code")
	qrCode: GroupChatQrCode {
		jid: ChatController.chatJid
	}
	qrCodeButton {
		description: qsTr("Share this group's chat address via QR code")
		onClicked: Utils.copyToClipboard(qrCode.source)
	}
	uriButton {
		description: qsTr("Share this group's chat address via text")
		onClicked: {
			Utils.copyToClipboard(Utils.groupChatUri(ChatController.chatJid))
			passiveNotification(qsTr("Group address copied to clipboard"))
		}
	}
	invitationButton {
		description: qsTr("Share this group's chat address via a web page with usage help")
		onClicked: Utils.copyToClipboard(Utils.invitationUrl(Utils.groupChatUri(ChatController.chatJid).toString()))
	}

	MobileForm.FormCard {
		Layout.fillWidth: true

		contentItem: ColumnLayout {
			spacing: 0

			MobileForm.FormCardHeader {
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
				currentIndex: indexOf(ChatController.rosterItem.notificationRule)
				onActivated: RosterModel.setNotificationRule(ChatController.accountJid, ChatController.chatJid, currentValue)
			}
		}
	}

	MobileForm.FormCard {
		Layout.fillWidth: true

		contentItem: ColumnLayout {
			spacing: 0

			MobileForm.FormCardHeader {
				title: qsTr("Privacy")
			}

			MobileForm.FormSwitchDelegate {
				text: qsTr("Send typing notifications")
				description: qsTr("Indicate when you have this conversation open, are typing and stopped typing")
				checked: ChatController.rosterItem.chatStateSendingEnabled
				onToggled: {
					RosterModel.setChatStateSendingEnabled(
						ChatController.accountJid,
						ChatController.chatJid,
						checked)
				}
			}

			MobileForm.FormSwitchDelegate {
				text: qsTr("Send read notifications")
				description: qsTr("Indicate which messages you have read")
				checked: ChatController.rosterItem.readMarkerSendingEnabled
				onToggled: {
					RosterModel.setReadMarkerSendingEnabled(
						ChatController.accountJid,
						ChatController.chatJid,
						checked)
				}
			}
		}
	}

	MobileForm.FormCard {
		Layout.fillWidth: true

		contentItem: ColumnLayout {
			id: removalArea
			spacing: 0
			enabled: !groupChatLeavingButton.busy && !groupChatDeletionButton.busy

			MobileForm.FormCardHeader {
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
					GroupChatController.leaveGroupChat(ChatController.accountJid, ChatController.chatJid)
				}
				busy: GroupChatController.busy
				busyText: qsTr("Leaving group chat…")

				Connections {
					target: GroupChatController

					function onGroupChatLeavingFailed(accountJid, groupChatJid, errorMessage) {
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
					GroupChatController.deleteGroupChat(ChatController.accountJid, ChatController.chatJid)
				}
				busy: GroupChatController.busy
				busyText: qsTr("Deleting group chat…")

				Connections {
					target: GroupChatController

					function onGroupChatDeletionFailed(accountJid, groupChatJid, errorMessage) {
						passiveNotification(qsTr("The group %1 could not be deleted%2").arg(groupChatJid).arg(errorMessage ? ": " + errorMessage : ""))
					}
				}
			}
		}
	}

	function openContactListView() {
		contactExpansionButton.toggle()
	}

	function openKeyAuthenticationUserListView() {
		keyAuthenticationButton.toggle()
	}
}
