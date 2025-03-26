// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2018 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Bhavy Airi <airiraghav@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan

import "elements"
import "elements/fields"
import "details"

Kirigami.GlobalDrawer {
	id: root

	property string selectedAccountJid

	topContent: [
		ColumnLayout {
			spacing: Kirigami.Units.largeSpacing
			Layout.margins: -3

			FormCard.FormCard {
				Layout.fillWidth: true

				FormCard.FormHeader {
					title: qsTr("Accounts")
				}

				InlineListView {
					model: [ AccountController.account.jid ]
					delegate: ColumnLayout {
						spacing: 0
						width: ListView.view.width

						FormCard.FormTextDelegate {
							id: accountArea

							property bool disconnected: Kaidan.connectionState === Enums.StateDisconnected
							property bool connected: Kaidan.connectionState === Enums.StateConnected

							background: FormCard.FormDelegateBackground { control: accountArea }
							leading: Avatar {
								jid: modelData
								name: AccountController.account.displayName
							}
							leadingPadding: 10
							// The placeholder text is used while "AccountController.account.displayName"
							// is not yet loaded to avoid a binding loop for the property
							// "implicitHeight".
							text: AccountController.account.displayName ? AccountController.account.displayName : " "
							description: Kaidan.connectionStateText
							descriptionItem {
								color: connected ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.textColor
								opacity: connected ? 1 : 0.5
							}
							trailing: Controls.Switch {
								checked: !accountArea.disconnected
								onToggled: accountArea.disconnected ? Kaidan.logIn() : Kaidan.logOut()
							}
							onClicked: {
								root.close()
								root.selectedAccountJid = modelData
								openViewFromGlobalDrawer(accountDetailsDialog, accountDetailsPage).jid = modelData
							}
						}

						FormCard.FormCard {
							visible: Kaidan.connectionError === ClientWorker.AuthenticationFailed
							Layout.fillWidth: true

							FormCardCustomContentArea {
								contentItem: RowLayout {
									PasswordField {
										id: passwordField
										labelText: qsTr("Password")
										placeholderText: qsTr("Enter your correct password")
										text: AccountController.account.passwordVisibility === Kaidan.PasswordVisible ? AccountController.account.password : ""
										invalidHintMayBeShown: true
										valid: credentialsValidator.isPasswordValid(text) && text !== AccountController.account.password
										enabled: !passwordBusyIndicator.visible
										inputField.onAccepted: passwordConfirmationButton.clicked()
									}

									Button {
										id: passwordConfirmationButton
										Controls.ToolTip.text: qsTr("Confirm password")
										icon.name: "emblem-ok-symbolic"
										visible: !passwordBusyIndicator.visible
										flat: !hovered
										Layout.preferredWidth: Layout.preferredHeight
										Layout.preferredHeight: passwordField.inputField.implicitHeight
										Layout.alignment: passwordField.invalidHint.visible ? Qt.AlignVCenter : Qt.AlignBottom
										onClicked: {
											if (!passwordField.valid) {
												passwordField.forceActiveFocus()
												passwordField.toggleHintForInvalidText()
											} else {
												AccountController.setAuthPassword(passwordField.text)
											}
										}
									}

									Controls.BusyIndicator {
										id: passwordBusyIndicator
										visible: Kaidan.connectionState !== Enums.StateDisconnected
										Layout.preferredWidth: passwordConfirmationButton.Layout.preferredWidth
										Layout.preferredHeight: Layout.preferredWidth
										Layout.alignment: passwordConfirmationButton.Layout.alignment
									}

									Connections {
										target: AccountController
										enabled: Kaidan.connectionError === ClientWorker.AuthenticationFailed

										function onAccountChanged() {
											// Try to log in with the entered password once it is stored.
											Kaidan.logIn()
										}
									}
								}
							}
						}

						Controls.Label {
							id: errorMessage
							visible: Kaidan.connectionError
							text: {
								const error = Kaidan.connectionError

								if (error === ClientWorker.NoError) {
									return ""
								}

								return Utils.connectionErrorMessage(error)
							}
							font.bold: true
							wrapMode: Text.WordWrap
							padding: 10
							Layout.margins: 10
							Layout.fillWidth: true
							background: RoundedRectangle {
								color: Kirigami.Theme.negativeBackgroundColor
							}
						}
					}
					implicitHeight: contentHeight
					Layout.fillWidth: true
				}
			}

			FormCard.FormCard {
				Layout.fillWidth: true

				FormCard.FormHeader {
					title: qsTr("Actions")
				}

				FormCard.FormButtonDelegate {
					text: qsTr("Add contact by QR code")
					icon.name: "view-barcode-qr"
					onClicked: openPageFromGlobalDrawer(contactAdditionQrCodePage)
				}

				FormCard.FormButtonDelegate {
					text: qsTr("Add contact by chat address")
					icon.name: "contact-new-symbolic"
					onClicked: openContactAdditionView()
				}

				FormCard.FormButtonDelegate {
					text: qsTr("Create group")
					icon.name: "resource-group-new"
					visible: Kaidan.connectionState === Enums.StateConnected && GroupChatController.groupChatCreationSupported
					onClicked: openViewFromGlobalDrawer(groupChatCreationDialog, groupChatCreationPage)
				}

				FormCard.FormButtonDelegate {
					text: qsTr("Join group")
					icon.name: "system-users-symbolic"
					visible: Kaidan.connectionState === Enums.StateConnected && GroupChatController.groupChatParticipationSupported
					onClicked: openViewFromGlobalDrawer(groupChatJoiningDialog, groupChatJoiningPage)
				}

				FormCard.FormButtonDelegate {
					id: publicGroupChatSearchButton
					text: qsTr("Search public groups")
					icon.name: "system-search-symbolic"
					onClicked: openOverlayFromGlobalDrawe(searchPublicGroupChatDialog)

					Shortcut {
						sequence: "Ctrl+G"
						onActivated: publicGroupChatSearchButton.clicked()
					}
				}

				FormCard.FormButtonDelegate {
					text: qsTr("About")
					icon.name: "help-about-symbolic"
					onClicked: openViewFromGlobalDrawer(aboutDialog, aboutPage)
				}
			}

			// placeholder to keep topContent at the top
			Item {
				Layout.fillHeight: true
			}
		}
	]
	onOpened: {
		if (Kaidan.connectionState === Enums.StateConnected) {
			// Request the user's current vCard which contains the user's nickname.
			Kaidan.vCardController.requestClientVCard()
		}
	}

	Component {
		id: accountDetailsDialog

		AccountDetailsDialog {
			jid: root.selectedAccountJid
		}
	}

	Component {
		id: accountDetailsPage

		AccountDetailsPage {
			jid: root.selectedAccountJid
		}
	}

	Component {
		id: avatarChangePage

		AvatarChangePage {}
	}

	Component {
		id: accountDetailsKeyAuthenticationPage

		AccountKeyAuthenticationPage {
			Component.onDestruction: openView(accountDetailsDialog, accountDetailsPage)
		}
	}

	Component {
		id: contactAdditionQrCodePage

		ContactAdditionQrCodePage {}
	}

	Component {
		id: groupChatCreationDialog

		GroupChatCreationDialog {
			accountJid: AccountController.account.jid
			nickname: AccountController.account.displayName
		}
	}

	Component {
		id: groupChatCreationPage

		GroupChatCreationPage {
			accountJid: AccountController.account.jid
			nickname: AccountController.account.displayName
		}
	}

	Component {
		id: searchPublicGroupChatDialog

		SearchPublicGroupChatDialog {}
	}

	Component {
		id: aboutDialog

		AboutDialog {}
	}

	Component {
		id: aboutPage

		AboutPage {}
	}

	function openContactAdditionView() {
		return openViewFromGlobalDrawer(contactAdditionDialog, contactAdditionPage)
	}

	function openOverlayFromGlobalDrawe(overlayComponent) {
		close()
		return openOverlay(overlayComponent)
	}

	function openPageFromGlobalDrawer(pageComponent) {
		close()
		return openPage(pageComponent)
	}

	function openViewFromGlobalDrawer(overlayComponent, pageComponent) {
		close()
		return openView(overlayComponent, pageComponent)
	}

	Connections {
		target: Kaidan

		function onCredentialsNeeded() {
			root.close()
		}

		function onXmppUriReceived(uriString) {
			const xmppUriPrefix = `xmpp:`
			root.openContactAdditionView().jid = uriString.substr(xmppUriPrefix.length)
		}
	}
}
