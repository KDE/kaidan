// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
// SPDX-FileCopyrightText: 2023 Bhavy Airi <airiraghav@gmail.com>
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
import "../elements/fields"
import "../settings"

DetailsContent {
	id: root

	automaticMediaDownloadsDelegate {
		model: [
			{
				display: qsTr("Never"),
				value: Account.AutomaticMediaDownloadsRule.Never
			},
			{
				display: qsTr("If personal data is shared"),
				value: Account.AutomaticMediaDownloadsRule.PresenceOnly
			},
			{
				display: qsTr("Always"),
				value: Account.AutomaticMediaDownloadsRule.Always
			}
		]
		textRole: "display"
		valueRole: "value"
		currentIndex: automaticMediaDownloadsDelegate.indexOf(Kaidan.settings.automaticMediaDownloadsRule)
		onActivated: {
			Kaidan.settings.automaticMediaDownloadsRule = automaticMediaDownloadsDelegate.currentValue
		}
	}
	mediaOverview {
		accountJid: root.jid
		chatJid: ""
	}
	vCardArea.visible: true
	vCardArea.enabled: accountRemovalArea.enabled
	vCardContentArea: [
		FormExpansionButton {
			checked: vCardRepeater.model.unsetEntriesProcessed
			onCheckedChanged: vCardRepeater.model.unsetEntriesProcessed = checked
		}
	]
	vCardRepeater {
		model: VCardModel {
			jid: root.jid
		}
		delegate: MobileForm.AbstractFormDelegate {
			id: vCardDelegate

			property bool editing: false

			contentItem: ColumnLayout {
				Controls.Label {
					text: model.value ? model.value : qsTr("Empty")
					color: model.value ? Kirigami.Theme.textColor : Kirigami.Theme.disabledTextColor
					font.italic: model.value ? false : true
					wrapMode: Text.WordWrap
					visible: !vCardDelegate.editing
					Layout.fillWidth: true
				}

				RowLayout {
					visible: vCardDelegate.editing

					Controls.TextField {
						id: vCardValueField
						text: model.value
						Layout.fillWidth: true
						onAccepted: vCardConfirmationButton.clicked()
					}

					Button {
						id: vCardConfirmationButton
						Controls.ToolTip.text: qsTr("Set value")
						icon.name: "emblem-ok-symbolic"
						visible: !vCardBusyIndicator.visible
						flat: !hovered
						Layout.preferredWidth: Layout.preferredHeight
						Layout.preferredHeight: customConnectionSettings.portField.implicitHeight
						Layout.alignment: Qt.AlignBottom
						onClicked: {
							vCardBusyIndicator.visible = true
							model.value = vCardValueField.text
							vCardDelegate.editing = false
							vCardBusyIndicator.visible = false
						}
					}

					Controls.BusyIndicator {
						id: vCardBusyIndicator
						visible: false
						Layout.preferredWidth: vCardConfirmationButton.Layout.preferredWidth
						Layout.preferredHeight: Layout.preferredWidth
						Layout.alignment: vCardConfirmationButton.Layout.alignment
					}
				}

				Controls.Label {
					text: model.key
					color: Kirigami.Theme.disabledTextColor
					font: Kirigami.Theme.smallFont
					textFormat: Text.PlainText
					wrapMode: Text.WordWrap
					Layout.fillWidth: true
				}
			}
			onClicked: {
				if (!editing) {
					vCardValueField.forceActiveFocus()
				}

				editing = !editing
			}
		}
	}
	encryptionArea: ColumnLayout {
		spacing: 0
		Component.onCompleted: {
			// Retrieve the own devices if they are not loaded yet.
			if (ChatController.accountJid !== root.jid) {
				EncryptionController.initializeAccountFromQml(root.jid)
			}
		}

		EncryptionWatcher {
			id: encryptionWatcher
			accountJid: root.jid
			jids: [root.jid]
		}

		MobileForm.FormCardHeader {
			title: qsTr("Encryption")
		}

		MobileForm.FormSwitchDelegate {
			text: qsTr("OMEMO 2")
			description: qsTr("End-to-end encryption with OMEMO 2 ensures that nobody else than you and your chat partners can read or modify the data you exchange.")
			checked: Kaidan.settings.encryption === Encryption.Omemo2
			// The switch is toggled by setting the user's preference on using encryption.
			// Note that 'checked' has already the value after the button is clicked.
			onClicked: {
				if (checked) {
					Kaidan.settings.encryption = Encryption.Omemo2
					RosterModel.setItemEncryption(root.jid, Encryption.Omemo2)
				} else {
					Kaidan.settings.encryption = Encryption.NoEncryption
					RosterModel.setItemEncryption(root.jid, Encryption.NoEncryption)
				}
			}
		}

		MobileForm.FormButtonDelegate {
			text: {
				if (!encryptionWatcher.hasUsableDevices) {
					if (encryptionWatcher.hasDistrustedDevices) {
						return qsTr("Verify <b>your</b> devices to encrypt for them")
					}

					if (ownResourcesWatcher.resourcesCount > 1) {
						return qsTr("<b>Your</b> other devices don't use OMEMO 2")
					}
				} else if (encryptionWatcher.hasAuthenticatableDevices) {
					if (encryptionWatcher.hasAuthenticatableDistrustedDevices) {
						return qsTr("Verify <b>your</b> devices to encrypt for them")
					}

					return qsTr("Verify <b>your</b> devices for maximum security")
				}

				return ""
			}
			icon.name: {
				if (!encryptionWatcher.hasUsableDevices) {
					if (encryptionWatcher.hasDistrustedDevices) {
						return "channel-secure-symbolic"
					}

					if (ownResourcesWatcher.resourcesCount > 1) {
						return "channel-insecure-symbolic"
					}
				} else if (encryptionWatcher.hasAuthenticatableDevices) {
					if (encryptionWatcher.hasAuthenticatableDistrustedDevices) {
						return "security-medium-symbolic"
					}

					return "security-high-symbolic"
				}

				return ""
			}
			visible: text
			enabled: encryptionWatcher.hasAuthenticatableDevices
			onClicked: root.openKeyAuthenticationPage(accountDetailsKeyAuthenticationPage).accountJid = root.jid

			UserResourcesWatcher {
				id: ownResourcesWatcher
				jid: root.jid
			}
		}
	}
	rosterGoupListView {
		enabled: accountRemovalArea.enabled
		delegate: MobileForm.AbstractFormDelegate {
			id: rosterGroupDelegate
			width: ListView.view.width
			onClicked: rosterGroupEditingButton.toggled()
			contentItem: RowLayout {
				Controls.Label {
					id: rosterGroupText
					text: modelData
					textFormat: Text.PlainText
					elide: Text.ElideRight
					visible: !rosterGroupTextField.visible
					Layout.preferredHeight: rosterGroupTextField.height
					Layout.fillWidth: true
					leftPadding: Kirigami.Units.smallSpacing * 1.5
				}

				Controls.TextField {
					id: rosterGroupTextField
					text: modelData
					visible: false
					Layout.fillWidth: true
					onAccepted: rosterGroupEditingButton.toggled()
				}

				Button {
					id: rosterGroupEditingButton
					text: qsTr("Change label…")
					icon.name: "document-edit-symbolic"
					display: Controls.AbstractButton.IconOnly
					checked: !rosterGroupText.visible
					flat: !hovered
					Controls.ToolTip.text: text
					// Ensure that the button can be used within "rosterGroupDelegate"
					// which acts as an overlay to toggle this button when clicked.
					// Otherwise, this button would be toggled by "rosterGroupDelegate"
					// and by this button's own visible area at the same time resulting
					// in resetting the toggling on each click.
					autoRepeat: true
					onToggled: {
						if (rosterGroupText.visible) {
							rosterGroupTextField.visible = true
							rosterGroupTextField.forceActiveFocus()
							rosterGroupTextField.selectAll()
						} else {
							rosterGroupTextField.visible = false

							if (rosterGroupTextField.text !== modelData) {
								RosterModel.updateGroup(modelData, rosterGroupTextField.text)
							}
						}
					}
				}

				Button {
					id: rosterGroupRemovalButton
					text: qsTr("Remove label")
					icon.name: "edit-delete-symbolic"
					display: Controls.AbstractButton.IconOnly
					flat: !rosterGroupDelegate.hovered
					Controls.ToolTip.text: text
					onClicked: {
						rosterGroupTextField.visible = false
						RosterModel.removeGroup(modelData)
					}
				}
			}
		}
	}
	qrCodeExpansionButton.description: qsTr("Share this account's chat address via QR code")
	qrCode: AccountQrCode {
		jid: root.jid
	}
	qrCodeButton {
		description: qsTr("Share this account's chat address via QR code")
		onClicked: Utils.copyToClipboard(qrCode.source)
	}
	uriButton {
		description: qsTr("Share this account's chat address via text")
		onClicked: {
			Utils.copyToClipboard(trustMessageUriGenerator.uri)
			passiveNotification(qsTr("Account address copied to clipboard"))
		}
	}
	invitationButton {
		description: qsTr("Share this account's chat address via a web page with usage help")
		onClicked: Utils.copyToClipboard(Utils.invitationUrl(trustMessageUriGenerator.uri))
	}

	MobileForm.FormCard {
		Layout.fillWidth: true
		contentItem: ColumnLayout {
			spacing: 0

			MobileForm.FormCardHeader {
				title: qsTr("Notifications")
			}

			FormComboBoxDelegate {
				id: contactNotificationDelegate
				text: qsTr("Incoming contact messages")
				description: qsTr("Show notification and play sound on message arrival from a contact")
				sheetUsed: root.sheet
				model: [
					{
						display: qsTr("Never"),
						value: Account.ContactNotificationRule.Never
					},
					{
						display: qsTr("If personal data is shared"),
						value: Account.ContactNotificationRule.PresenceOnly
					},
					{
						display: qsTr("Always"),
						value: Account.ContactNotificationRule.Always
					}
				]
				textRole: "display"
				valueRole: "value"
				currentIndex: contactNotificationDelegate.indexOf(AccountManager.account.contactNotificationRule)
				onActivated: AccountManager.setContactNotificationRule(root.jid, contactNotificationDelegate.currentValue)
			}

			FormComboBoxDelegate {
				id: groupChatNotificationDelegate
				text: qsTr("Incoming group messages")
				description: qsTr("Show notification and play sound on message arrival from a group")
				sheetUsed: root.sheet
				model: [
					{
						display: qsTr("Never"),
						value: Account.GroupChatNotificationRule.Never
					},
					{
						display: qsTr("On mention"),
						value: Account.GroupChatNotificationRule.Mentioned
					},
					{
						display: qsTr("Always"),
						value: Account.GroupChatNotificationRule.Always
					}
				]
				textRole: "display"
				valueRole: "value"
				currentIndex: groupChatNotificationDelegate.indexOf(AccountManager.account.groupChatNotificationRule)
				onActivated: AccountManager.setGroupChatNotificationRule(root.jid, groupChatNotificationDelegate.currentValue)
			}
		}
	}

	MobileForm.FormCard {
		id: providerArea

		readonly property url providerUrl: providerListModel.providerFromBareJid(root.jid).chosenWebsite
		readonly property var chatSupportList: providerListModel.providerFromBareJid(root.jid).chosenChatSupport
		readonly property var groupChatSupportList: providerListModel.providerFromBareJid(root.jid).chosenGroupChatSupport

		visible: providerUrl.toString() || chatSupportList.length || groupChatSupportList.length
		enabled: accountRemovalArea.enabled
		Layout.fillWidth: true
		contentItem: ColumnLayout {
			spacing: 0

			ProviderListModel {
				id: providerListModel
			}

			MobileForm.FormCardHeader {
				title: qsTr("Provider")
			}

			MobileForm.FormButtonDelegate {
				text: qsTr("Visit website")
				description: qsTr("Open your provider's website in a web browser")
				visible: providerArea.providerUrl.toString()
				onClicked: Qt.openUrlExternally(providerArea.providerUrl)
			}

			MobileForm.FormButtonDelegate {
				text: qsTr("Copy website address")
				description: qsTr("Copy your provider's web address to the clipboard")
				visible: providerArea.providerUrl.toString()
				onClicked: {
					Utils.copyToClipboard(providerArea.providerUrl)
					passiveNotification(qsTr("Website address copied to clipboard"))
				}
			}

			MobileForm.FormButtonDelegate {
				text: qsTr("Open support chat")
				description: qsTr("Start chat with your provider's support contact")
				visible: providerArea.chatSupportList.length > 0
				onClicked: {
					if (providerArea.chatSupportList.length === 1) {
						let contactAdditionView = openView(contactAdditionDialog, contactAdditionPage)
						contactAdditionView.jid = providerArea.chatSupportList[0]
						contactAdditionView.name = qsTr("Support")

						if (root.sheet) {
							root.sheet.close()
						}
					} else {
						chatSupportListView.visible = !chatSupportListView.visible
					}
				}
			}

			ListView {
				id: chatSupportListView
				visible: false
				implicitHeight: contentHeight
				Layout.fillWidth: true
				clip: true
				model: Array.from(providerArea.chatSupportList)
				delegate: MobileForm.FormCard {
					width: ListView.view.width
					Kirigami.Theme.colorSet: Kirigami.Theme.Window
					contentItem: MobileForm.AbstractFormDelegate {
						background: Item {}
						horizontalPadding: 0
						verticalPadding: 0
						contentItem: MobileForm.FormButtonDelegate {
							text: qsTr("Support %1").arg(index + 1)
							description: modelData
							onClicked: {
								let contactAdditionView = openView(contactAdditionDialog, contactAdditionPage)
								contactAdditionView.jid = modelData
								contactAdditionView.name = text

								if (root.sheet) {
									root.sheet.close()
								}
							}
						}
					}
				}
			}

			MobileForm.FormButtonDelegate {
				text: qsTr("Open support group")
				description: qsTr("Join your provider's public support group")
				visible: providerArea.groupChatSupportList.length > 0
				onClicked: {
					if (providerArea.groupChatSupportList.length === 1) {
						Qt.openUrlExternally(Utils.groupChatUri(providerArea.groupChatSupportList[0]))
					} else {
						groupChatSupportListView.visible = !groupChatSupportListView.visible
					}
				}
			}

			ListView {
				id: groupChatSupportListView
				visible: false
				implicitHeight: contentHeight
				Layout.fillWidth: true
				clip: true
				model: Array.from(providerArea.groupChatSupportList)
				delegate: MobileForm.FormCard {
					width: ListView.view.width
					Kirigami.Theme.colorSet: Kirigami.Theme.Window
					contentItem: MobileForm.AbstractFormDelegate {
						background: Item {}
						horizontalPadding: 0
						verticalPadding: 0
						contentItem: MobileForm.FormButtonDelegate {
							text: qsTr("Group Support %1").arg(index + 1)
							description: modelData
							onClicked: Qt.openUrlExternally(Utils.groupChatUri(modelData))
						}
					}
				}
			}
		}
	}

	MobileForm.FormCard {
		enabled: accountRemovalArea.enabled
		Layout.fillWidth: true
		contentItem: ColumnLayout {
			spacing: 0

			MobileForm.FormCardHeader {
				title: qsTr("Blocked Chat Addresses")
			}

			MobileForm.FormSectionText {
				text: qsTr("Block a specific user (e.g., user@example.org) or all users of the same server (e.g., example.org)")
				visible: blockingExpansionButton.checked
			}

			ListView {
				id: blockingListView
				model: BlockingModel {}
				visible: blockingExpansionButton.checked
				implicitHeight: contentHeight
				Layout.fillWidth: true
				header: MobileForm.FormCard {
					width: ListView.view.width
					Kirigami.Theme.colorSet: Kirigami.Theme.Window
					contentItem: MobileForm.AbstractFormDelegate {
						background: Item {}
						contentItem: RowLayout {
							spacing: Kirigami.Units.largeSpacing * 3

							Controls.Label {
								text: qsTr("You must be connected to block or unblock chat addresses")
								visible: Kaidan.connectionState !== Enums.StateConnected
								Layout.fillWidth: true
							}

							Controls.TextField {
								id: blockingTextField
								placeholderText: qsTr("user@example.org")
								visible: Kaidan.connectionState === Enums.StateConnected
								enabled: !blockingAction.loading
								Layout.fillWidth: true
								onAccepted: blockingButton.clicked()
								onVisibleChanged: {
									if (visible) {
										clear()
										forceActiveFocus()
									}
								}
							}

							Button {
								id: blockingButton
								Controls.ToolTip.text: qsTr("Block chat address")
								icon.name: "list-add-symbolic"
								visible: !blockingAction.loading && Kaidan.connectionState === Enums.StateConnected
								enabled: blockingTextField.text.length
								flat: !hovered
								Layout.preferredWidth: Layout.preferredHeight
								Layout.preferredHeight: blockingTextField.implicitHeight
								Layout.rightMargin: Kirigami.Units.largeSpacing
								onClicked: {
									const jid = blockingTextField.text
									if (blockingListView.model.contains(jid)) {
										blockingTextField.clear()
									} else if (enabled) {
										blockingAction.block(jid)
										blockingTextField.clear()
									} else {
										blockingTextField.forceActiveFocus()
									}
								}
							}

							Controls.BusyIndicator {
								visible: blockingAction.loading
								Layout.preferredWidth: blockingButton.Layout.preferredWidth
								Layout.preferredHeight: Layout.preferredWidth
								Layout.rightMargin: blockingButton.Layout.rightMargin
							}
						}
					}
				}
				section.property: "type"
				section.delegate: ListViewSectionDelegate {}
				delegate: MobileForm.AbstractFormDelegate {
					id: blockingDelegate
					width: ListView.view.width
					onClicked: blockingEditingButton.toggled()
					contentItem: RowLayout {
						Controls.Label {
							id: blockingText
							text: model.jid
							textFormat: Text.PlainText
							elide: Text.ElideMiddle
							visible: !blockingEditingTextField.visible
							Layout.preferredHeight: blockingEditingTextField.height
							Layout.fillWidth: true
							leftPadding: Kirigami.Units.smallSpacing * 1.5
						}

						Controls.TextField {
							id: blockingEditingTextField
							text: model.jid
							visible: false
							Layout.fillWidth: true
							onAccepted: blockingEditingButton.toggled()
						}

						Button {
							id: blockingEditingButton
							text: qsTr("Change chat address…")
							icon.name: "document-edit-symbolic"
							display: Controls.AbstractButton.IconOnly
							checked: !blockingText.visible
							flat: !hovered
							Controls.ToolTip.text: text
							// Ensure that the button can be used within "blockingDelegate"
							// which acts as an overlay to toggle this button when clicked.
							// Otherwise, this button would be toggled by "blockingDelegate"
							// and by this button's own visible area at the same time resulting
							// in resetting the toggling on each click.
							autoRepeat: true
							onToggled: {
								if (blockingText.visible) {
									blockingEditingTextField.visible = true
									blockingEditingTextField.forceActiveFocus()
									blockingEditingTextField.selectAll()
								} else {
									blockingEditingTextField.visible = false

									if (blockingEditingTextField.text !== model.jid) {
										blockingAction.block(blockingEditingTextField.text)
										blockingAction.unblock(model.jid)
									}
								}
							}
						}

						Button {
							text: qsTr("Unblock")
							icon.name: "edit-delete-symbolic"
							visible: Kaidan.connectionState === Enums.StateConnected
							display: Controls.AbstractButton.IconOnly
							flat: !blockingDelegate.hovered
							Controls.ToolTip.text: text
							onClicked: blockingAction.unblock(model.jid)
						}
					}
				}
			}

			FormExpansionButton {
				id: blockingExpansionButton
			}
		}
	}

	MobileForm.FormCard {
		id: notesAdditionArea
		visible: !RosterModel.hasItem(root.jid)
		enabled: accountRemovalArea.enabled
		Layout.fillWidth: true
		contentItem: ColumnLayout {
			spacing: 0

			MobileForm.FormCardHeader {
				title: qsTr("Notes")
			}

			// TODO: Find a solution (hide button or add local-only chat) for servers not allowing to add oneself to the roster (such as Prosody)
			MobileForm.FormButtonDelegate {
				text: qsTr("Add chat for notes")
				description: qsTr("Add a chat for synchronizing your notes across all your devices")
				icon.name: "note-symbolic"
				onClicked: {
					Kaidan.client.rosterManager.addContactRequested(root.jid)
					Kaidan.openChatPageRequested(root.jid, root.jid)
				}

				Connections {
					target: RosterModel

					function onAddItemRequested(item) {
						if (item.jid === root.jid) {
							notesAdditionArea.visible = false
						}
					}

					function onRemoveItemRequested(accountJid, jid) {
						if (accountJid === jid) {
							notesAdditionArea.visible = true
						}
					}
				}
			}
		}
	}

	MobileForm.FormCard {
		visible: Kaidan.serverFeaturesCache.inBandRegistrationSupported
		enabled: accountRemovalArea.enabled
		Layout.fillWidth: true
		contentItem: ColumnLayout {
			spacing: 0

			MobileForm.FormCardHeader {
				title: qsTr("Password Change")
			}

			MobileForm.FormSectionText {
				text: qsTr("Change your password. You need to enter the new password on all your other devices!")
			}

			MobileForm.FormCard {
				Layout.fillWidth: true
				Kirigami.Theme.colorSet: Kirigami.Theme.Window
				contentItem: MobileForm.AbstractFormDelegate {
					background: Item {}
					contentItem: ColumnLayout {
						Component.onCompleted: {
							passwordVerificationField.initialize()
							passwordField.initialize()
						}

						PasswordField {
							id: passwordVerificationField
							labelText: qsTr("Current password")
							placeholderText: qsTr("Enter your current password")
							invalidHintText: qsTr("Enter correct password")
							visible: Kaidan.settings.passwordVisibility !== Kaidan.PasswordVisible
							enabled: !passwordBusyIndicator.visible
							Layout.rightMargin: passwordChangeButton.Layout.preferredWidth + passwordButtonFieldArea.spacing
							onTextChanged: {
								valid = text === AccountManager.password
								toggleHintForInvalidText()
							}
							inputField.onAccepted: passwordChangeButton.clicked()

							function initialize() {
								showPassword = false
								invalidHintMayBeShown = false
								inputField.clear()
							}
						}

						RowLayout {
							id: passwordButtonFieldArea

							PasswordField {
								id: passwordField
								labelText: passwordVerificationField.visible ? qsTr("New password") : qsTr("Password")
								placeholderText: qsTr("Enter your new password")
								invalidHintText: qsTr("Enter different password to change it")
								invalidHintMayBeShown: true
								enabled: !passwordBusyIndicator.visible
								onTextChanged: {
									valid = credentialsValidator.isPasswordValid(text) && text !== AccountManager.password
									toggleHintForInvalidText()
								}
								inputField.onAccepted: passwordChangeButton.clicked()

								function initialize() {
									showPassword = false
									text = Kaidan.settings.passwordVisibility !== Kaidan.PasswordVisible ? "" : AccountManager.password

									// Avoid showing a hint on initial setting.
									invalidHint.visible = false
								}
							}

							Button {
								id: passwordChangeButton
								Controls.ToolTip.text: qsTr("Change password")
								icon.name: "emblem-ok-symbolic"
								visible: !passwordBusyIndicator.visible
								flat: !hovered
								Layout.preferredWidth: Layout.preferredHeight
								Layout.preferredHeight: passwordField.inputField.implicitHeight
								Layout.alignment: passwordField.invalidHint.visible ? Qt.AlignVCenter : Qt.AlignBottom
								onClicked: {
									if (passwordVerificationField.visible && !passwordVerificationField.valid) {
										passwordVerificationField.forceActiveFocus()
									} else if (!passwordField.valid) {
										passwordField.forceActiveFocus()
										passwordField.toggleHintForInvalidText()
									} else {
										passwordBusyIndicator.visible = true
										Kaidan.client.registrationManager.changePasswordRequested(passwordField.text)
									}
								}
							}

							Controls.BusyIndicator {
								id: passwordBusyIndicator
								visible: false
								Layout.preferredWidth: passwordChangeButton.Layout.preferredWidth
								Layout.preferredHeight: Layout.preferredWidth
								Layout.alignment: passwordChangeButton.Layout.alignment
							}
						}

						Controls.Label {
							id: passwordChangeErrorMessage
							visible: false
							font.bold: true
							wrapMode: Text.WordWrap
							padding: 10
							Layout.fillWidth: true
							background: RoundedRectangle {
								color: Kirigami.Theme.negativeBackgroundColor
							}
						}

						Connections {
							target: Kaidan

							function onPasswordChangeFailed(errorMessage) {
								passwordBusyIndicator.visible = false
								passwordChangeErrorMessage.visible = true
								passwordChangeErrorMessage.text = qsTr("Failed to change password: %1").arg(errorMessage)
							}

							function onPasswordChangeSucceeded() {
								passwordBusyIndicator.visible = false
								passwordChangeErrorMessage.visible = false
								passiveNotification(qsTr("Password changed successfully"))
							}
						}
					}
				}
			}
		}
	}

	MobileForm.FormCard {
		visible: Kaidan.settings.passwordVisibility !== Kaidan.PasswordInvisible
		Layout.fillWidth: true
		contentItem: ColumnLayout {
			spacing: 0

			MobileForm.FormCardHeader {
				title: qsTr("Password Visibility")
			}

			MobileForm.FormSectionText {
				text: qsTr("Configure this device to not expose your password for changing it or switching to another device. If you want to change your password or use your account on another device later, <b>consider storing the password somewhere else. This cannot be undone!</b>")
			}

			MobileForm.FormButtonDelegate {
				text: qsTr("Don't show password as text")
				description: qsTr("Allow to add additional devices using the login QR code but never show the password")
				icon.name: "security-medium-symbolic"
				visible: Kaidan.settings.passwordVisibility === Kaidan.PasswordVisible
				onClicked: passwordRemovalFromQrCodeConfirmationButton.visible = !passwordRemovalFromQrCodeConfirmationButton.visible
			}

			MobileForm.FormButtonDelegate {
				id: passwordRemovalFromQrCodeConfirmationButton
				text: qsTr("Confirm")
				visible: false
				Layout.leftMargin: Kirigami.Units.smallSpacing * 7
				onClicked: {
					Kaidan.settings.passwordVisibility = Kaidan.PasswordVisibleQrOnly
					passwordField.initialize()
				}
			}

			MobileForm.FormButtonDelegate {
				text: qsTr("Don't expose password in any way")
				description: qsTr("Neither allow to add additional devices using the login QR code nor show the password")
				icon.name: "security-high-symbolic"
				visible: Kaidan.settings.passwordVisibility !== Kaidan.PasswordInvisible
				onClicked: passwordRemovalConfirmationButton.visible = !passwordRemovalConfirmationButton.visible
			}

			MobileForm.FormButtonDelegate {
				id: passwordRemovalConfirmationButton
				text: qsTr("Confirm")
				visible: false
				Layout.leftMargin: Kirigami.Units.smallSpacing * 7
				onClicked: {
					const oldPasswordVisibility = Kaidan.settings.passwordVisibility
					Kaidan.settings.passwordVisibility = Kaidan.PasswordInvisible

					// Do not initialize passwordField when the password is already hidden.
					if (oldPasswordVisibility === Kaidan.PasswordVisible) {
						passwordField.initialize()
					}
				}
			}
		}
	}

	MobileForm.FormCard {
		enabled: accountRemovalArea.enabled
		Layout.fillWidth: true
		contentItem: ColumnLayout {
			spacing: 0

			MobileForm.FormCardHeader {
				title: qsTr("Connection")
			}

			MobileForm.FormSectionText {
				text: qsTr("Configure the hostname and port to connect to (empty fields for default values)")
			}

			MobileForm.FormCard {
				Layout.fillWidth: true
				Kirigami.Theme.colorSet: Kirigami.Theme.Window
				contentItem: MobileForm.AbstractFormDelegate {
					background: Item {}
					contentItem: ColumnLayout {
						id: connectionSettings

						RowLayout {
							CustomConnectionSettings {
								id: customConnectionSettings
								confirmationButton: connectionSettingsButton
							}

							Button {
								id: connectionSettingsButton
								Controls.ToolTip.text: qsTr("Change connection settings")
								icon.name: "emblem-ok-symbolic"
								visible: !connectionSettingsBusyIndicator.visible
								flat: !hovered
								Layout.preferredWidth: Layout.preferredHeight
								Layout.preferredHeight: customConnectionSettings.portField.implicitHeight
								Layout.alignment: Qt.AlignBottom
								onClicked: {
									if (customConnectionSettings.hostField.text === AccountManager.host && customConnectionSettings.portField.value === AccountManager.port) {
										connectionSettingsErrorMessage.text = qsTr("Enter different connection settings to change them")
										connectionSettingsErrorMessage.visible = true
									} else {
										connectionSettingsBusyIndicator.visible = true

										// Reset the error message in case of previous button clicking without changed entered settings.
										if (Kaidan.connectionError === ClientWorker.NoError) {
											connectionSettingsErrorMessage.visible = false
										}

										if (Kaidan.connectionState === Enums.StateDisconnected) {
											connectionSettings.logIn()
										} else {
											Kaidan.logOut()
										}
									}
								}
							}

							Controls.BusyIndicator {
								id: connectionSettingsBusyIndicator
								visible: false
								Layout.preferredWidth: connectionSettingsButton.Layout.preferredWidth
								Layout.preferredHeight: Layout.preferredWidth
								Layout.alignment: connectionSettingsButton.Layout.alignment
							}
						}

						Controls.Label {
							id: connectionSettingsErrorMessage
							visible: false
							font.bold: true
							wrapMode: Text.WordWrap
							padding: 10
							Layout.fillWidth: true
							background: RoundedRectangle {
								color: Kirigami.Theme.negativeBackgroundColor
							}
						}

						Connections {
							target: Kaidan

							function onConnectionErrorChanged() {
								// Skip connection error changes not invoked via connectionSettings by checking whether connectionSettingsBusyIndicator is visible.
								if (Kaidan.connectionError === ClientWorker.NoError) {
									connectionSettingsErrorMessage.visible = false
								} else {
									connectionSettingsErrorMessage.visible = true
									connectionSettingsErrorMessage.text = qsTr("Connection settings could not be changed: %1").arg(Utils.connectionErrorMessage(Kaidan.connectionError))
								}
							}

							function onConnectionStateChanged() {
								// Skip connection state changes not invoked via connectionSettings by checking whether connectionSettingsBusyIndicator is visible.
								if (connectionSettingsBusyIndicator.visible) {
									if (Kaidan.connectionState === Enums.StateDisconnected) {
										if (Kaidan.connectionError === ClientWorker.NoError) {
											connectionSettings.logIn()
										} else {
											connectionSettingsBusyIndicator.visible = false
										}
									} else if (Kaidan.connectionState === Enums.StateConnected) {
										connectionSettingsBusyIndicator.visible = false
										passiveNotification(qsTr("Connection settings changed"))
									}
								}
							}
						}

						function logIn() {
							AccountManager.host = customConnectionSettings.hostField.text
							AccountManager.port = customConnectionSettings.portField.value
							Kaidan.logIn()
						}
					}
				}
			}
		}
	}

	MobileForm.FormCard {
		enabled: accountRemovalArea.enabled
		Layout.fillWidth: true
		contentItem: ColumnLayout {
			spacing: 0

			MobileForm.FormCardHeader {
				title: qsTr("Migration")
			}

			ColumnLayout {
				spacing: 0

				BusyIndicatorFormButton {
					id: migrateButton
					idleText: qsTr("Migrate account")
					busyText: qsTr("Preparing account migration…")
					description: busy ? qsTr("That can take long") : qsTr("Migrate account data (except chat history) to another account. Your current account will be removed from this app. Back up your credentials and chat history if needed!")
					idleIconSource: "edit-copy-symbolic"
					onClicked: {
						busy = true
						Kaidan.startAccountMigration()
					}

					Connections {
						target: Kaidan
						enabled: root.sheet

						function onOpenStartPageRequested() {
							root.sheet.close()
						}
					}
				}
			}
		}
	}

	MobileForm.FormCard {
		Layout.fillWidth: true
		contentItem: ColumnLayout {
			id: accountRemovalArea
			spacing: 0
			enabled: !accountRemovalButtonArea.busy && !accountDeletionButtonArea.busy

			MobileForm.FormCardHeader {
				title: qsTr("Removal")
			}

			ConfirmationFormButtonArea {
				id: accountRemovalButtonArea
				button {
					text: qsTr("Remove from Kaidan")
					description: qsTr("Remove account from this app. Back up your credentials and chat history if needed!")
					icon.name: "edit-delete-symbolic"
					icon.color: Kirigami.Theme.neutralTextColor
				}
				confirmationButton.onClicked: AccountManager.deleteAccountFromClient()
				busyText: qsTr("Removing account…")
			}

			ConfirmationFormButtonArea {
				id: accountDeletionButtonArea
				visible: Kaidan.serverFeaturesCache.inBandRegistrationSupported
				button {
					text: qsTr("Delete completely")
					description: qsTr("Delete account from provider. You will not be able to use your account again!")
					icon.name: "edit-delete-symbolic"
					icon.color: Kirigami.Theme.negativeTextColor
				}
				confirmationButton.onClicked: AccountManager.deleteAccountFromClientAndServer()
				busyText: qsTr("Deleting account…")

				Connections {
					target: AccountManager

					function onAccountDeletionFromClientAndServerFailed(errorMessage) {
						accountDeletionButtonArea.busy = false
						passiveNotification(qsTr("Your account could not be deleted from the server. Therefore, it was also not removed from this app: %1").arg(error));
					}
				}
			}
		}
	}

	AccountTrustMessageUriGenerator {
		id: trustMessageUriGenerator
		jid: root.jid
	}
}
