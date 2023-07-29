// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
// SPDX-FileCopyrightText: 2023 Bhavy Airi <airiraghav@gmail.com>
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
import "../elements/fields"
import "../settings"

DetailsContent {
	id: root
	mediaOverview {
		accountJid: AccountManager.jid
		chatJid: ""
	}
	vCardArea: [
		FormExpansionButton {
			id: vCardExpansionButton
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
						flat: true
						Layout.preferredWidth: Layout.preferredHeight
						Layout.preferredHeight: customConnectionSettings.portField.implicitHeight
						Layout.alignment: Qt.AlignBottom
						onHoveredChanged: {
							if (hovered) {
								flat = false
							} else {
								flat = true
							}
						}
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
	rosterGroupArea: ColumnLayout {
		MobileForm.FormCard {
			Layout.fillWidth: true
			contentItem: ColumnLayout {
				spacing: 0

				MobileForm.FormCardHeader {
					title: qsTr("Labels")
				}

				ListView {
					id: rosterGroupListView
					model: RosterModel.groups
					visible: rosterGroupExpansionButton.checked
					implicitHeight: contentHeight
					Layout.fillWidth: true
					delegate: MobileForm.AbstractFormDelegate {
						id: rosterGroupDelegate
						width: ListView.view.width
						onClicked: rosterGroupEditingButton.toggled()
						contentItem: RowLayout {
							Controls.Label {
								id: rosterGroupText
								text: modelData
								textFormat: Text.PlainText
								maximumLineCount: 1
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

				FormExpansionButton {
					id: rosterGroupExpansionButton
				}
			}
		}
	}
	encryptionArea: ColumnLayout {
		spacing: 0
		Component.onCompleted: {
			// Retrieve the own devices if they are not loaded yet.
			if (MessageModel.currentAccountJid != root.jid) {
				Kaidan.client.omemoManager.initializeChatRequested(root.jid)
			}
		}

		OmemoWatcher {
			id: omemoWatcher
			jid: root.jid
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
				if (!omemoWatcher.usableOmemoDevices.length) {
					if (omemoWatcher.distrustedOmemoDevices.length) {
						return qsTr("Scan the QR codes of <b>your</b> devices to encrypt for them")
					} else if (ownResourcesWatcher.resourcesCount > 1) {
						return qsTr("<b>Your</b> other devices don't use OMEMO 2")
					}
				} else if (omemoWatcher.authenticatableOmemoDevices.length) {
					if (omemoWatcher.authenticatableOmemoDevices.length === omemoWatcher.distrustedOmemoDevices.length) {
						return qsTr("Scan the QR codes of <b>your</b> devices to encrypt for them")
					}

					return qsTr("Scan the QR codes of <b>your</b> devices for maximum security")
				}

				return ""
			}
			icon.name: {
				if (!omemoWatcher.usableOmemoDevices.length) {
					if (omemoWatcher.distrustedOmemoDevices.length) {
						return "channel-secure-symbolic"
					} else if (ownResourcesWatcher.resourcesCount > 1) {
						return "channel-insecure-symbolic"
					}
				} else if (omemoWatcher.authenticatableOmemoDevices.length) {
					if (omemoWatcher.authenticatableOmemoDevices.length === omemoWatcher.distrustedOmemoDevices.length) {
						return "security-medium-symbolic"
					}

					return "security-high-symbolic"
				}

				return ""
			}
			visible: text
			enabled: omemoWatcher.authenticatableOmemoDevices.length
			onClicked: pageStack.layers.push(qrCodePage, { isForOwnDevices: true })

			UserResourcesWatcher {
				id: ownResourcesWatcher
				jid: root.jid
			}
		}
	}

	MobileForm.FormCard {
		id: providerArea

		readonly property string providerUrl: {
			const domain = root.jid.split('@')[1]
			const provider = providerListModel.provider(domain)

			return providerListModel.chooseWebsite(provider.websites)
		}

		readonly property var chatSupportList: providerListModel.providerFromBareJid(root.jid).chatSupport
		readonly property var groupChatSupportList: providerListModel.providerFromBareJid(root.jid).groupChatSupport

		Layout.fillWidth: true
		visible: providerUrl  || chatSupportList.length || groupChatSupportList.length
		contentItem: ColumnLayout {
			spacing: 0

			ProviderListModel {
				id: providerListModel
			}

			ChatSupportSheet {
				id: chatSupportSheet
				chatSupportList: providerArea.chatSupportList
			}

			MobileForm.FormCardHeader {
				title: qsTr("Provider")
			}

			MobileForm.FormButtonDelegate {
				text: qsTr("Visit website")
				description: qsTr("Open your provider's website in a web browser")
				visible: providerArea.providerUrl
				onClicked: Qt.openUrlExternally(providerArea.providerUrl)
			}

			MobileForm.FormButtonDelegate {
				text: qsTr("Copy website address")
				description: qsTr("Copy your provider's web address to the clipboard")
				visible: providerArea.providerUrl
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
						let contactAdditionContainer = openView(contactAdditionDialog, contactAdditionPage)
						contactAdditionContainer.jid = providerArea.chatSupportList[0]
						contactAdditionContainer.name = qsTr("Support")

						if (root.sheet) {
							root.sheet.close()
						}
					} else {
						chatSupportSheet.open()
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
						chatSupportSheet.isGroupChatSupportSheet = true

						if (!chatSupportSheet.sheetOpen) {
							chatSupportSheet.open()
						}
					}
				}
			}
		}
	}

	MobileForm.FormCard {
		visible: Kaidan.serverFeaturesCache.inBandRegistrationSupported
		Layout.fillWidth: true

		contentItem: ColumnLayout {
			spacing: 0

			MobileForm.FormCardHeader {
				title: qsTr("Password Change")
			}

			MobileForm.FormSectionText {
				text: qsTr("Change your password. You need to enter the new password on all your other devices!")
			}

			MobileForm.AbstractFormDelegate {
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
							text = ""
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
							flat: true
							Layout.preferredWidth: Layout.preferredHeight
							Layout.preferredHeight: passwordField.inputField.implicitHeight
							Layout.alignment: passwordField.invalidHint.visible ? Qt.AlignVCenter : Qt.AlignBottom
							onHoveredChanged: {
								if (hovered) {
									flat = false
								} else {
									flat = true
								}
							}
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
						background: Rectangle {
							color: Kirigami.Theme.negativeBackgroundColor
							radius: roundedCornersRadius
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

	MobileForm.FormCard {
		visible: Kaidan.settings.passwordVisibility !== Kaidan.PasswordInvisible
		Layout.fillWidth: true

		contentItem: ColumnLayout {
			spacing: 0

			MobileForm.FormCardHeader {
				title: qsTr("Password Security")
			}

			MobileForm.FormSectionText {
				text: qsTr("Configure this device to not expose your password for changing it or switching to another device. If you want to change your password or use your account on another device later, <b>consider storing the password somewhere else. This cannot be undone!</b>")
			}

			MobileForm.FormButtonDelegate {
				text: qsTr("Don't show password as text")
				visible: Kaidan.settings.passwordVisibility === Kaidan.PasswordVisible
				description: qsTr("Allow to add additional devices using the login QR code but never show the password")
				icon.name: "security-medium-symbolic"
				onClicked: {
					Kaidan.settings.passwordVisibility = Kaidan.PasswordVisibleQrOnly
					passwordField.initialize()
				}
			}

			MobileForm.FormButtonDelegate {
				text: qsTr("Don't expose password in any way")
				visible: Kaidan.settings.passwordVisibility !== Kaidan.PasswordInvisible
				description: qsTr("Neither allow to add additional devices using the login QR code nor show the password")
				icon.name: "security-high-symbolic"
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
		Layout.fillWidth: true

		contentItem: ColumnLayout {
			spacing: 0

			MobileForm.FormCardHeader {
				title: qsTr("Connection")
			}

			MobileForm.FormSectionText {
				text: qsTr("Configure the hostname and port to connect to (empty fields for default values)")
			}

			MobileForm.AbstractFormDelegate {
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
							flat: true
							Layout.preferredWidth: Layout.preferredHeight
							Layout.preferredHeight: customConnectionSettings.portField.implicitHeight
							Layout.alignment: Qt.AlignBottom
							onHoveredChanged: {
								if (hovered) {
									flat = false
								} else {
									flat = true
								}
							}
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
						background: Rectangle {
							color: Kirigami.Theme.negativeBackgroundColor
							radius: roundedCornersRadius
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

	MobileForm.FormCard {
		Layout.fillWidth: true

		contentItem: ColumnLayout {
			spacing: 0

			MobileForm.FormCardHeader {
				title: qsTr("Removal")
			}

			ColumnLayout {
				spacing: 0

				MobileForm.FormButtonDelegate {
					id: removalButton
					text: qsTr("Remove from Kaidan")
					description: qsTr("Remove account from this app. Back up your credentials and chat history if needed!")
					icon.name: "edit-delete-symbolic"
					icon.color: Kirigami.Theme.neutralTextColor
					checkable: true
					onToggled: contactRemovalCorfirmationButton.visible = !contactRemovalCorfirmationButton.visible
				}

				MobileForm.FormButtonDelegate {
					id: contactRemovalCorfirmationButton
					text: qsTr("Confirm")
					visible: false
					Layout.leftMargin: Kirigami.Units.largeSpacing * 6
					onClicked: {
						visible = false
						removalButton.enabled = false
						Kaidan.deleteAccountFromClient()
					}
				}
			}

			ColumnLayout {
				spacing: 0

				MobileForm.FormButtonDelegate {
					id: deletionButton
					text: qsTr("Delete completely")
					description: qsTr("Delete account from provider. You will not be able to use your account again!")
					icon.name: "edit-delete-symbolic"
					icon.color: Kirigami.Theme.negativeTextColor
					checkable: true
					onToggled: contactDeletionCorfirmationButton.visible = !contactDeletionCorfirmationButton.visible
				}

				MobileForm.FormButtonDelegate {
					id: contactDeletionCorfirmationButton
					text: qsTr("Confirm")
					visible: false
					Layout.leftMargin: Kirigami.Units.largeSpacing * 6
					onClicked: {
						visible = false
						removalButton.enabled = false
						Kaidan.deleteAccountFromClientAndServer()
					}
				}
			}
		}
	}
}
