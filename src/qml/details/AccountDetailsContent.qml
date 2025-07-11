// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
// SPDX-FileCopyrightText: 2023 Bhavy Airi <airiraghav@gmail.com>
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
import "../elements/fields"

DetailsContent {
	id: root
	mediaAreaText: qsTr("You can share media up to %1.").arg(account.settings.httpUploadLimitText)
	automaticMediaDownloadsDelegate {
		model: [
			{
				display: qsTr("Never"),
				value: AccountSettings.AutomaticMediaDownloadsRule.Never
			},
			{
				display: qsTr("If personal data is shared"),
				value: AccountSettings.AutomaticMediaDownloadsRule.PresenceOnly
			},
			{
				display: qsTr("Always"),
				value: AccountSettings.AutomaticMediaDownloadsRule.Always
			}
		]
		textRole: "display"
		valueRole: "value"
		currentIndex: automaticMediaDownloadsDelegate.indexOf(root.account.settings.automaticMediaDownloadsRule)
		onActivated: {
			root.account.settings.automaticMediaDownloadsRule = automaticMediaDownloadsDelegate.currentValue
		}
	}
	mediaOverview {
		accountJid: root.account.settings.jid
		chatJid: ""
	}
	vCardArea.enabled: accountRemovalArea.enabled
	vCardRepeater {
		model: VCardModel {
			connection: root.account.connection
			vCardController: root.account.vCardController
			jid: root.account.settings.jid
		}
		delegate: FormCard.AbstractFormDelegate {
			id: vCardDelegate

			property bool editing: false

			enabled: root.account.settings.enabled
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
	vCardExpansionButton {
		checked: vCardRepeater.model.unsetEntriesProcessed
		onCheckedChanged: vCardRepeater.model.unsetEntriesProcessed = vCardExpansionButton.checked
	}
	encryptionArea {
		delegates: [
			FormCard.FormHeader {
				title: qsTr("Encryption")
			},

			FormCard.FormSwitchDelegate {
				text: qsTr("OMEMO 2")
				description: qsTr("End-to-end encryption with OMEMO 2 ensures that nobody else than you and your chat partners can read or modify the data you exchange.")
				checked: root.account.settings.encryption === Encryption.Omemo2
				// The switch is toggled by setting the user's preference on using encryption.
				// Note that 'checked' has already the value after the button is clicked.
				onClicked: {
					if (checked) {
						root.account.settings.encryption = Encryption.Omemo2
						RosterModel.setItemEncryption(root.account.settings.jid, Encryption.Omemo2)
					} else {
						root.account.settings.encryption = Encryption.NoEncryption
						RosterModel.setItemEncryption(root.account.settings.jid, Encryption.NoEncryption)
					}
				}
			},

			AccountKeyAuthenticationButton {
				presenceCache: root.account.presenceCache
				jid: root.account.settings.jid
				encryptionWatcher: EncryptionWatcher {
					encryptionController: root.account.encryptionController
					accountJid: root.account.settings.jid
					jids: [root.account.settings.jid]
				}
				onClicked: root.openKeyAuthenticationPage(accountDetailsKeyAuthenticationPage)
			}
		]
		Component.onCompleted: root.account.encryptionController.initializeAccount()
	}
	rosterGoupListView {
		enabled: accountRemovalArea.enabled
		delegate: FormCard.AbstractFormDelegate {
			id: rosterGroupDelegate
			width: ListView.view.width
			enabled: root.account.settings.enabled
			onClicked: rosterGroupEditingButton.toggled()
			contentItem: RowLayout {
				Controls.Label {
					id: rosterGroupText
					text: modelData
					textFormat: Text.PlainText
					elide: Text.ElideRight
					visible: !rosterGroupTextField.visible
					verticalAlignment: Text.AlignVCenter
					leftPadding: rosterGroupTextField.leftPadding
					Layout.preferredHeight: rosterGroupTextField.height
					Layout.fillWidth: true
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
					checkable: true
					checked: !rosterGroupText.visible
					visible: root.account.settings.enabled
					flat: !hovered
					// Ensure that the button can be used within "rosterGroupDelegate"
					// which acts as an overlay to toggle this button when clicked.
					// Otherwise, this button would be toggled by "rosterGroupDelegate"
					// and by this button's own visible area at the same time resulting
					// in resetting the toggling on each click.
					autoRepeat: true
					Controls.ToolTip.text: text
					onToggled: {
						if (rosterGroupText.visible) {
							rosterGroupTextField.visible = true
							rosterGroupTextField.forceActiveFocus()
							rosterGroupTextField.selectAll()
						} else {
							rosterGroupTextField.visible = false

							if (rosterGroupTextField.text !== modelData) {
								root.account.rosterController.updateGroup(modelData, rosterGroupTextField.text)
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
					visible: root.account.settings.enabled
					Controls.ToolTip.text: text
					onClicked: {
						rosterGroupTextField.visible = false
						root.account.rosterController.removeGroup(modelData)
					}
				}
			}
		}
	}
	qrCodeExpansionButton.description: qsTr("Share this account's chat address via QR code")
	qrCode: AccountQrCode {
		uriGenerator: trustMessageUriGenerator
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

	UserDevicesArea {
		account: root.account
		jid: root.account.settings.jid
	}

	FormCard.FormCard {
		Layout.fillWidth: true

		FormCard.FormHeader {
			title: qsTr("Notifications")
		}

		FormComboBoxDelegate {
			text: qsTr("Incoming contact messages")
			description: qsTr("Show notification and play sound on message arrival from a contact")
			model: [
				{
					display: qsTr("Never"),
					value: AccountSettings.ContactNotificationRule.Never
				},
				{
					display: qsTr("If personal data is shared"),
					value: AccountSettings.ContactNotificationRule.PresenceOnly
				},
				{
					display: qsTr("Always"),
					value: AccountSettings.ContactNotificationRule.Always
				}
			]
			textRole: "display"
			valueRole: "value"
			currentIndex: indexOf(root.account.settings.contactNotificationRule)
			onActivated: root.account.settings.contactNotificationRule = currentValue
		}

		FormComboBoxDelegate {
			text: qsTr("Incoming group messages")
			description: qsTr("Show notification and play sound on message arrival from a group")
			model: [
				{
					display: qsTr("Never"),
					value: AccountSettings.GroupChatNotificationRule.Never
				},
				{
					display: qsTr("On mention"),
					value: AccountSettings.GroupChatNotificationRule.Mentioned
				},
				{
					display: qsTr("Always"),
					value: AccountSettings.GroupChatNotificationRule.Always
				}
			]
			textRole: "display"
			valueRole: "value"
			currentIndex: indexOf(root.account.settings.groupChatNotificationRule)
			onActivated: root.account.settings.groupChatNotificationRule = currentValue
		}
	}

	FormCard.FormCard {
		Layout.fillWidth: true

		FormCard.FormHeader {
			title: qsTr("Locations")
		}

		FormCard.FormSectionText {
			text: qsTr("For displaying locations in Kaidan, online data from the third party provider OpenStreetMap is used. The data is downloaded from it after submitting the location.")
		}

		FormCard.FormSwitchDelegate {
			id: geoLocationMapPreviewDelegate
			text: qsTr("Location map previews")
			description: qsTr("Display previews of locations")
			checked: root.account.settings.geoLocationMapPreviewEnabled
			// Note that 'checked' has already the value after the button is clicked.
			onClicked: root.account.settings.geoLocationMapPreviewEnabled = checked
		}

		FormComboBoxDelegate {
			id: geoLocationMapServiceDelegate
			text: qsTr("Opening locations")
			description: qsTr("How to open locations")
			model: [
				{
					display: qsTr("System default"),
					value: AccountSettings.GeoLocationMapService.System
				},
				{
					display: qsTr("Kaidan"),
					value: AccountSettings.GeoLocationMapService.InApp
				},
				{
					display: qsTr("Web"),
					value: AccountSettings.GeoLocationMapService.Web
				}
			]
			textRole: "display"
			valueRole: "value"
			currentIndex: geoLocationMapServiceDelegate.indexOf(root.account.settings.geoLocationMapService)
			onActivated: root.account.settings.geoLocationMapService = geoLocationMapServiceDelegate.currentValue
		}
	}

	FormCard.FormCard {
		id: providerArea

		readonly property url providerUrl: providerListModel.providerFromBareJid(root.account.settings.jid).chosenWebsite
		readonly property var chatSupportList: providerListModel.providerFromBareJid(root.account.settings.jid).chosenChatSupport
		readonly property var groupChatSupportList: providerListModel.providerFromBareJid(root.account.settings.jid).chosenGroupChatSupport

		visible: providerUrl.toString() || chatSupportList.length || groupChatSupportList.length
		enabled: accountRemovalArea.enabled
		Layout.fillWidth: true

		ProviderListModel {
			id: providerListModel
		}

		FormCard.FormHeader {
			title: qsTr("Provider")
		}

		FormCard.FormButtonDelegate {
			text: qsTr("Visit website")
			description: qsTr("Open your provider's website in a web browser")
			visible: providerArea.providerUrl.toString()
			onClicked: Qt.openUrlExternally(providerArea.providerUrl)
		}

		FormCard.FormButtonDelegate {
			text: qsTr("Copy website address")
			description: qsTr("Copy your provider's web address to the clipboard")
			visible: providerArea.providerUrl.toString()
			onClicked: {
				Utils.copyToClipboard(providerArea.providerUrl)
				passiveNotification(qsTr("Website address copied to clipboard"))
			}
		}

		FormCard.FormButtonDelegate {
			text: qsTr("Open support chat")
			description: qsTr("Start chat with your provider's support contact")
			visible: root.account.settings.enabled && providerArea.chatSupportList.length
			onClicked: {
				if (providerArea.chatSupportList.length === 1) {
					let contactAdditionView = openView(contactAdditionDialog, contactAdditionPage)
					contactAdditionView.jid = providerArea.chatSupportList[0]
					contactAdditionView.name = qsTr("Support")

					if (root.dialog) {
						root.dialog.close()
					}
				} else {
					chatSupportListView.visible = !chatSupportListView.visible
				}
			}
		}

		InlineListView {
			id: chatSupportListView
			visible: false
			implicitHeight: contentHeight
			Layout.fillWidth: true
			model: Array.from(providerArea.chatSupportList)
			delegate: FormCard.FormCard {
				width: ListView.view.width
				Kirigami.Theme.colorSet: Kirigami.Theme.Window

				FormCard.AbstractFormDelegate {
					background: null
					horizontalPadding: 0
					verticalPadding: 0
					contentItem: FormCard.FormButtonDelegate {
						text: qsTr("Support %1").arg(index + 1)
						description: modelData
						onClicked: {
							let contactAdditionView = openView(contactAdditionDialog, contactAdditionPage)
							contactAdditionView.jid = modelData
							contactAdditionView.name = text

							if (root.dialog) {
								root.dialog.close()
							}
						}
					}
				}
			}
		}

		FormCard.FormButtonDelegate {
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

		InlineListView {
			id: groupChatSupportListView
			visible: false
			implicitHeight: contentHeight
			Layout.fillWidth: true
			model: Array.from(providerArea.groupChatSupportList)
			delegate: FormCard.FormCard {
				width: ListView.view.width
				Kirigami.Theme.colorSet: Kirigami.Theme.Window

				FormCard.AbstractFormDelegate {
					background: null
					horizontalPadding: 0
					verticalPadding: 0
					contentItem: FormCard.FormButtonDelegate {
						text: qsTr("Group Support %1").arg(index + 1)
						description: modelData
						onClicked: Qt.openUrlExternally(Utils.groupChatUri(modelData))
					}
				}
			}
		}
	}

	FormCard.FormCard {
		enabled: accountRemovalArea.enabled
		visible: root.account.settings.enabled || blockingListView.count
		Layout.fillWidth: true

		FormCard.FormHeader {
			title: qsTr("Blocked Chat Addresses")
		}

		FormCard.FormSectionText {
			text: qsTr("Block a specific user (e.g., user@example.org) or all users of the same server (e.g., example.org)")
			visible: root.account.settings.enabled && blockingExpansionButton.checked
		}

		InlineListView {
			id: blockingListView
			model: BlockingModel {
				blockingController: account.blockingController
			}
			visible: blockingExpansionButton.checked
			implicitHeight: contentHeight
			Layout.fillWidth: true
			header: Loader {
				sourceComponent: root.account.settings.enabled ? blockingComponent : null
				width: ListView.view.width

				Component {
					id: blockingComponent

					FormCard.FormCard {
						Kirigami.Theme.colorSet: Kirigami.Theme.Window

						FormCard.AbstractFormDelegate {
							background: null
							contentItem: RowLayout {
								spacing: Kirigami.Units.largeSpacing * 3

								Controls.Label {
									text: qsTr("You must be connected to block or unblock chat addresses")
									visible: root.account.connection.state !== Enums.StateConnected
									Layout.fillWidth: true
								}

								Controls.TextField {
									id: blockingTextField
									placeholderText: qsTr("user@example.org")
									visible: root.account.connection.state === Enums.StateConnected
									enabled: !root.account.blockingController.busy
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
									visible: !root.account.blockingController.busy
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
											root.account.blockingController.block(jid)
											blockingTextField.clear()
										} else {
											blockingTextField.forceActiveFocus()
										}
									}
								}

								Controls.BusyIndicator {
									visible: root.account.blockingController.busy
									Layout.preferredWidth: blockingButton.Layout.preferredWidth
									Layout.preferredHeight: Layout.preferredWidth
									Layout.rightMargin: blockingButton.Layout.rightMargin
								}
							}
						}
					}
				}
			}
			section.property: "type"
			section.delegate: ListViewSectionDelegate {}
			delegate: FormCard.AbstractFormDelegate {
				id: blockingDelegate
				width: ListView.view.width
				enabled: root.account.settings.enabled
				onClicked: blockingEditingButton.toggled()
				contentItem: RowLayout {
					Controls.Label {
						id: blockingText
						text: model.jid
						textFormat: Text.PlainText
						elide: Text.ElideMiddle
						verticalAlignment: Text.AlignVCenter
						visible: !blockingEditingTextField.visible
						Layout.preferredHeight: blockingEditingTextField.height
						Layout.fillWidth: true
						leftPadding: blockingEditingTextField.leftPadding
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
						checkable: true
						checked: !blockingText.visible
						flat: !hovered
						visible: root.account.settings.enabled
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
									root.account.blockingController.block(blockingEditingTextField.text)
									root.account.blockingController.unblock(model.jid)
								}
							}
						}
					}

					Button {
						text: qsTr("Unblock")
						icon.name: "edit-delete-symbolic"
						display: Controls.AbstractButton.IconOnly
						flat: !blockingDelegate.hovered
						visible: root.account.settings.enabled
						Controls.ToolTip.text: text
						onClicked: root.account.blockingController.unblock(model.jid)
					}
				}
			}
		}

		FormExpansionButton {
			id: blockingExpansionButton
		}
	}

	FormCard.FormCard {
		id: notesAdditionArea
		visible: root.account.settings.enabled && !RosterModel.hasItem(root.account.settings.jid, root.account.settings.jid)
		enabled: accountRemovalArea.enabled
		Layout.fillWidth: true

		FormCard.FormHeader {
			title: qsTr("Notes")
		}

		// TODO: Find a solution (hide button or add local-only chat) for servers not allowing to add oneself to the roster (such as Prosody)
		BusyIndicatorFormButton {
			id: notesAdditionButton
			idleText: qsTr("Add chat for notes")
			busyText: qsTr("Adding notes chat…")
			description: qsTr("Add a chat for synchronizing your notes across all your devices")
			idleIconSource: "note-symbolic"
			onClicked: {
				busy = true
				root.account.rosterController.addContact(root.account.settings.jid)
			}

			Connections {
				target: root.account.rosterController

				function onContactAdditionFailed(jid) {
					if (jid === root.account.settings.jid) {
						notesAdditionButton.busy = false
						passiveNotification(qsTr("The notes chat could not be added to your contact list because your server does not support that"))
					}
				}
			}

			Connections {
				target: RosterModel

				function onItemAdded(item) {
					const accountJid = item.accountJid

					if (accountJid === root.account.settings.jid && item.jid === root.account.settings.jid) {
						if (notesAdditionButton.busy) {
							MainController.openChatPageRequested(accountJid, accountJid)
						} else {
							notesAdditionArea.visible = false
						}
					}
				}

				function onItemRemoved(item) {
					if (item.accountJid === root.account.settings.jid && item.jid === root.account.settings.jid) {
						notesAdditionArea.visible = true
					}
				}
			}
		}
	}

	FormCard.FormCard {
		visible: root.account.settings.enabled && root.account.settings.inBandRegistrationFeaturesSupported
		enabled: accountRemovalArea.enabled
		Layout.fillWidth: true

		FormCard.FormHeader {
			title: qsTr("Password Change")
		}

		FormCard.FormSectionText {
			text: qsTr("Change your password. You need to enter the new password on all your other devices!")
		}

		FormCardCustomContentArea {
			contentItem: ColumnLayout {
				PasswordField {
					id: passwordVerificationField
					labelText: qsTr("Current password")
					placeholderText: qsTr("Enter your current password")
					invalidHintText: qsTr("Enter correct password")
					valid: text === root.account.settings.password
					visible: root.account.settings.passwordVisibility !== AccountSettings.PasswordVisibility.Visible
					enabled: !passwordBusyIndicator.visible
					Layout.rightMargin: passwordChangeButton.Layout.preferredWidth + passwordButtonFieldArea.spacing
					inputField.onAccepted: passwordChangeButton.clicked()
				}

				RowLayout {
					id: passwordButtonFieldArea

					PasswordField {
						id: passwordField
						labelText: passwordVerificationField.visible ? qsTr("New password") : qsTr("Password")
						placeholderText: qsTr("Enter your new password")
						text: passwordVerificationField.visible ? "" : root.account.settings.password
						invalidHintText: qsTr("Enter different password to change it")
						valid: credentialsValidator.isPasswordValid(text) && text !== root.account.settings.password
						enabled: !passwordBusyIndicator.visible
						inputField.onAccepted: passwordChangeButton.clicked()
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
								passwordField.invalidHintMayBeShown = false
								passwordBusyIndicator.visible = true
								root.account.registrationController.changePassword(passwordField.text)
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
					font.weight: Font.Medium
					wrapMode: Text.WordWrap
					padding: 10
					Layout.fillWidth: true
					background: RoundedRectangle {
						color: Kirigami.Theme.negativeBackgroundColor
					}
				}

				Connections {
					target: root.account.registrationController

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

	FormCard.FormCard {
		visible: root.account.settings.passwordVisibility !== AccountSettings.PasswordVisibility.Invisible
		Layout.fillWidth: true

		FormCard.FormHeader {
			title: qsTr("Password Visibility")
		}

		FormCard.FormSectionText {
			text: qsTr("Configure this device to not expose your password for changing it or switching to another device. If you want to change your password or use your account on another device later, <b>consider storing the password somewhere else. This cannot be undone!</b>")
		}

		ConfirmationFormButtonArea {
			visible: root.account.settings.passwordVisibility === AccountSettings.PasswordVisibility.Visible
			button {
				text: qsTr("Don't show password as text")
				description: qsTr("Allow to add additional devices using the login QR code but never show the password")
				icon.name: "security-medium-symbolic"
			}
			confirmationButton.onClicked: {
				busy = true
				root.account.settings.passwordVisibility = AccountSettings.PasswordVisibility.OnlyVisibleViaQrCode
			}
			busyText: qsTr("Removing password from text…")
		}

		ConfirmationFormButtonArea {
			visible: root.account.settings.passwordVisibility !== AccountSettings.PasswordVisibility.Invisible
			button {
				text: qsTr("Don't expose password in any way")
				description: qsTr("Neither allow to add additional devices using the login QR code nor show the password")
				icon.name: "security-high-symbolic"
			}
			confirmationButton.onClicked: {
				busy = true
				root.account.settings.passwordVisibility = AccountSettings.PasswordVisibility.Invisible
			}
			busyText: qsTr("Removing password from text and QR code…")
		}
	}

	FormCard.FormCard {
		visible: root.account.settings.enabled
		enabled: accountRemovalArea.enabled
		Layout.fillWidth: true

		FormCard.FormHeader {
			title: qsTr("Connection")
		}

		FormCard.FormSectionText {
			text: qsTr("Configure the hostname and port to connect to (empty fields for default values)")
		}

		FormCardCustomContentArea {
			contentItem: ColumnLayout {
				id: connectionSettings

				RowLayout {
					CustomConnectionSettings {
						id: customConnectionSettings
						account: root.account
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
							if (customConnectionSettings.hostField.text === root.account.settings.host && customConnectionSettings.portField.value === root.account.settings.port) {
								connectionSettingsErrorMessage.text = qsTr("Enter different connection settings to change them")
								connectionSettingsErrorMessage.visible = true
							} else {
								connectionSettingsBusyIndicator.visible = true

								// Reset the error message in case of previous button clicking without changed entered settings.
								if (root.account.connection.error === ClientWorker.NoError) {
									connectionSettingsErrorMessage.visible = false
								}

								if (root.account.connection.state === Enums.StateDisconnected) {
									connectionSettings.logIn()
								} else {
									root.account.logOut()
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
					font.weight: Font.Medium
					wrapMode: Text.WordWrap
					padding: 10
					Layout.fillWidth: true
					background: RoundedRectangle {
						color: Kirigami.Theme.negativeBackgroundColor
					}
				}

				Connections {
					target: root.account.connection

					function onErrorChanged() {
						// Skip connection error changes not invoked via connectionSettings by checking whether connectionSettingsBusyIndicator is visible.
						if (root.account.connection.error === ClientWorker.NoError) {
							connectionSettingsErrorMessage.visible = false
						} else {
							connectionSettingsErrorMessage.visible = true
							connectionSettingsErrorMessage.text = qsTr("Connection settings could not be changed: %1").arg(root.account.connection.errorText)
						}
					}

					function onStateChanged() {
						// Skip connection state changes not invoked via connectionSettings by checking whether connectionSettingsBusyIndicator is visible.
						if (connectionSettingsBusyIndicator.visible) {
							if (root.account.connection.state === Enums.StateDisconnected) {
								if (root.account.connection.error === ClientWorker.NoError) {
									connectionSettings.logIn()
								} else {
									connectionSettingsBusyIndicator.visible = false
								}
							} else if (root.account.connection.state === Enums.StateConnected) {
								connectionSettingsBusyIndicator.visible = false
								passiveNotification(qsTr("Connection settings changed"))
							}
						}
					}
				}

				function logIn() {
					root.account.settings.host = customConnectionSettings.hostField.text
					root.account.settings.port = customConnectionSettings.portField.value
					root.account.connection.logIn()
				}
			}
		}
	}

	FormCard.FormCard {
		enabled: accountRemovalArea.enabled
		Layout.fillWidth: true

		FormCard.FormHeader {
			title: qsTr("Switch Device")
		}

		FormCard.FormSectionText {
			text: qsTr("Never show the information in this section to anyone else. It would allow unlimited access to your account!")
			visible: root.account.settings.passwordVisibility !== AccountSettings.PasswordVisibility.Invisible
		}

		FormCard.FormButtonDelegate {
			id: loginQrCodeExpansionButton
			text: qsTr("Show login QR code")
			description: qsTr("Scan the QR code by another device to log in on it")
			icon.name: "view-barcode-qr"
			checkable: true
		}

		FormCard.AbstractFormDelegate {
			visible: loginQrCodeExpansionButton.checked
			background: FormCard.FormDelegateBackground {
				control: parent
				color: secondaryBackgroundColor
			}
			contentItem: LoginQrCode {
				accountSettings: root.account.settings
			}
			Layout.preferredWidth: parent.width
			Layout.preferredHeight: Layout.preferredWidth
		}

		FormCard.FormButtonDelegate {
			id: credentialsExpansionButton
			text: qsTr("Show credentials")
			description: qsTr("Enter the credentials in another device to log in on it")
			icon.name: "password-show-on"
			checkable: true
		}

		CopyFormTextDelegate {
			text: qsTr("Chat address")
			description: root.account.settings.jid
			visible: credentialsExpansionButton.checked
		}

		CopyFormTextDelegate {
			text: qsTr("Password")
			description: root.account.settings.password
			visible: credentialsExpansionButton.checked && root.account.settings.passwordVisibility === AccountSettings.PasswordVisibility.Visible
		}
	}

	FormCard.FormCard {
		visible: root.account.settings.enabled
		enabled: accountRemovalArea.enabled
		Layout.fillWidth: true

		FormCard.FormHeader {
			title: qsTr("Migration")
		}

		ColumnLayout {
			spacing: 0

			BusyIndicatorFormButton {
				id: migrateButton
				idleText: qsTr("Migrate account")
				busyText: qsTr("Preparing account migration…")
				busy: AccountController.migrating
				description: busy ? qsTr("This may take a while") : qsTr("Migrate account data (except chat history) to another account. Your current account will be removed from this app. Back up your credentials and chat history if needed!")
				idleIconSource: "edit-copy-symbolic"
				onClicked: AccountController.startMigration(root.account)

				Connections {
					target: MainController
					enabled: root.dialog

					function onOpenStartPageRequested() {
						root.dialog.close()
					}
				}
			}
		}
	}

	FormCard.FormCard {
		id: accountRemovalArea
		Layout.fillWidth: true
		enabled: !accountRemovalButtonArea.busy && !accountDeletionButtonArea.busy
		visible: root.account.settings.enabled

		FormCard.FormHeader {
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
			confirmationButton.onClicked: {
				busy = true
				root.account.registrationController.deleteAccountFromClient()
			}
			busyText: qsTr("Removing account…")
		}

		ConfirmationFormButtonArea {
			id: accountDeletionButtonArea
			visible: root.account.settings.inBandRegistrationFeaturesSupported
			button {
				text: qsTr("Delete completely")
				description: qsTr("Delete account from provider. You will not be able to use your account again!")
				icon.name: "edit-delete-symbolic"
				icon.color: Kirigami.Theme.negativeTextColor
			}
			confirmationButton.onClicked: {
				busy = true
				root.account.registrationController.deleteAccountFromClientAndServer()
			}
			busyText: qsTr("Deleting account…")

			Connections {
				target: root.account.registrationController

				function onAccountDeletionFromClientAndServerFailed(errorMessage) {
					accountDeletionButtonArea.busy = false
					passiveNotification(qsTr("Your account could not be deleted from the server. Therefore, it was also not removed from this app: %1").arg(errorMessage));
				}
			}
		}
	}

	AccountTrustMessageUriGenerator {
		id: trustMessageUriGenerator
		encryptionController: root.account.encryptionController
		jid: root.account.settings.jid
	}
}
