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

	property AccountSelectionDialog accountSelectionDialog
	property Account selectedAccount
	property Component nextOverlayToOpen
	property Component nextPageToOpen

	property var accounts: AccountController.accounts

	property int enabledAccountCount: {
		let count = 0

		for (let i in accounts) {
			let account = accounts[i]

			if (account.settings.enabled) {
				count++
			}
		}

		return count
	}

	enabled: pageStack.visibleItems.length && pageStack.visibleItems[0] instanceof RosterPage && (pageStack.wideMode || pageStack.trailingVisibleItem instanceof RosterPage)
	topContent: [
		ColumnLayout {
			spacing: Kirigami.Units.largeSpacing
			Layout.rightMargin: 1

			FormCard.FormCard {
				Layout.fillWidth: true

				FormCard.FormHeader {
					title: qsTr("Accounts")
				}

				InlineListView {
					model: root.accounts
					delegate: ColumnLayout {
						spacing: 0
						width: ListView.view.width

						FormCard.FormTextDelegate {
							id: accountArea

							property bool disconnected: modelData.connection.state === Enums.StateDisconnected
							property bool connected: modelData.connection.state === Enums.StateConnected

							background: FormCard.FormDelegateBackground { control: accountArea }
							leading: Avatar {
								jid: modelData.settings.jid
								name: modelData.settings.displayName
							}
							leadingPadding: 10
							// The placeholder text is used while "modelData.settings.displayName"
							// is not yet loaded to avoid a binding loop for the property
							// "implicitHeight".
							text: modelData.settings.displayName ? modelData.settings.displayName : " "
							description: modelData.connection.stateText
							descriptionItem {
								color: connected ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.textColor
								opacity: connected ? 1 : 0.5
							}
							trailing: Controls.Switch {
								checkable: false
								checked: modelData.settings.enabled
								onClicked: checked ? modelData.disable() : modelData.enable()
							}
							onClicked: {
								root.selectedAccount = modelData
								openView(accountDetailsDialog, accountDetailsPage)
								root.close()
							}
						}

						Controls.Label {
							id: errorMessage
							visible: modelData.connection.error
							text: modelData.connection.errorText
							font.weight: Font.Medium
							wrapMode: Text.WordWrap
							padding: 10
							Layout.margins: 10
							Layout.fillWidth: true
							background: RoundedRectangle {
								color: Kirigami.Theme.negativeBackgroundColor
							}
						}

						FormCard.FormCard {
							visible: modelData.connection.error === ClientWorker.AuthenticationFailed
							Layout.fillWidth: true

							FormCardCustomContentArea {
								contentItem: RowLayout {
									PasswordField {
										id: passwordField
										labelText: qsTr("Password")
										placeholderText: qsTr("Enter your correct password")
										text: modelData.settings.passwordVisibility === AccountSettings.PasswordVisibility.Visible ? modelData.settings.password : ""
										valid: credentialsValidator.isPasswordValid(text) && text !== modelData.settings.password
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
											if (passwordField.valid) {
												modelData.settings.password = passwordField.text
												passwordField.invalidHintMayBeShown = false
												modelData.connection.logIn()
											} else {
												passwordField.invalidHintMayBeShown = true
												passwordField.forceActiveFocus()
											}
										}
									}

									Controls.BusyIndicator {
										id: passwordBusyIndicator
										visible: modelData.connection.state !== Enums.StateDisconnected
										Layout.preferredWidth: passwordConfirmationButton.Layout.preferredWidth
										Layout.preferredHeight: Layout.preferredWidth
										Layout.alignment: passwordConfirmationButton.Layout.alignment
									}
								}
							}
						}
					}
					implicitHeight: contentHeight
					Layout.fillWidth: true
				}

				FormAdditionButton {
					onClicked: {
						root.close()
						openStartPage(true)
					}
				}
			}

			FormCard.FormCard {
				Layout.fillWidth: true

				FormCard.FormHeader {
					title: qsTr("Actions")
				}

				FormCard.FormSectionText {
					text: root.accounts.length === 1 ? qsTr("Enable your account to use all available actions") : qsTr("Enable an account to use all available actions")
					visible: !root.enabledAccountCount
				}

				FormCard.FormButtonDelegate {
					text: qsTr("Add contact by QR code")
					icon.name: "view-barcode-qr"
					enabled: root.enabledAccountCount
					onClicked: root.openPageFromGlobalDrawer(contactAdditionQrCodePage)
				}

				FormCard.FormButtonDelegate {
					text: qsTr("Add contact by chat address")
					icon.name: "contact-new-symbolic"
					enabled: root.enabledAccountCount
					onClicked: root.openContactAdditionView()
				}

				FormCard.FormButtonDelegate {
					text: enabled || !root.enabledAccountCount ? qsTr("Create group") : qsTr("Create group (unsupported)")
					icon.name: "resource-group-new"
					enabled: {
						const accounts = root.accounts
						let supportedAccountCount = 0

						for (let i in accounts) {
							let account = accounts[i]

							if (account.settings.enabled && account.groupChatController.groupChatCreationSupported) {
								supportedAccountCount++
							}
						}

						return supportedAccountCount
					}
					onClicked: {
						root.openViewFromGlobalDrawer(groupChatCreationDialog, groupChatCreationPage)

						if (root.accountSelectionDialog) {
							root.accountSelectionDialog.groupChatCreationSupported = true
						}
					}
				}

				FormCard.FormButtonDelegate {
					text: enabled || !root.enabledAccountCount ? qsTr("Join group") : qsTr("Join group (unsupported)")
					icon.name: "system-users-symbolic"
					enabled: {
						const accounts = root.accounts
						let supportedAccountCount = 0

						for (let i in accounts) {
							let account = accounts[i]

							if (account.settings.enabled && account.groupChatController.groupChatParticipationSupported) {
								supportedAccountCount++
							}
						}

						return supportedAccountCount
					}
					onClicked: {
						root.openViewFromGlobalDrawer(groupChatJoiningDialog, groupChatJoiningPage)

						if (root.accountSelectionDialog) {
							root.accountSelectionDialog.groupChatParticipationSupported = true
						}
					}
				}

				FormCard.FormButtonDelegate {
					id: publicGroupChatSearchButton
					text: qsTr("Search public groups")
					icon.name: "system-search-symbolic"
					onClicked: {
						openOverlay(publicGroupChatSearchDialog)
						root.close()
					}

					Shortcut {
						sequence: "Ctrl+G"
						onActivated: publicGroupChatSearchButton.clicked()
					}
				}

				FormCard.FormButtonDelegate {
					text: qsTr("About")
					icon.name: "help-about-symbolic"
					onClicked: {
						openView(aboutDialog, aboutPage)
						root.close()
					}
				}
			}

			// placeholder to keep topContent at the top
			Item {
				Layout.fillHeight: true
			}
		}
	]
	onSelectedAccountChanged: {
		if (!selectedAccount) {
			return
		}

		if (nextOverlayToOpen) {
			openOverlay(nextOverlayToOpen)
		} else if (nextPageToOpen) {
			openPage(nextPageToOpen)
		}
	}
	onOpened: {
		selectedAccount = null
		nextOverlayToOpen = null
		nextPageToOpen = null

		for (let i in accounts) {
			let account = accounts[i]

			if (account.connection.state === Enums.StateConnected) {
				// Request the user's current vCard which contains the user's nickname.
				account.vCardController.requestOwnVCard()
			}
		}
	}

	Component {
		id: accountDetailsDialog

		AccountDetailsDialog {
			account: root.selectedAccount
		}
	}

	Component {
		id: accountDetailsPage

		AccountDetailsPage {
			account: root.selectedAccount
		}
	}

	Component {
		id: avatarChangePage

		AvatarChangePage {
			account: root.selectedAccount
		}
	}

	Component {
		id: accountDetailsKeyAuthenticationPage

		AccountKeyAuthenticationPage {
			account: root.selectedAccount
			Component.onDestruction: openView(accountDetailsDialog, accountDetailsPage)
		}
	}

	Component {
		id: accountSelectionDialog

		AccountSelectionDialog {
			onAccountSelected: account => {
				root.selectedAccount = account
			}
			Component.onCompleted: root.accountSelectionDialog = this
			Component.onDestruction: root.accountSelectionDialog = null
		}
	}

	Component {
		id: contactAdditionQrCodePage

		ContactAdditionQrCodePage {
			account: root.selectedAccount
		}
	}

	Component {
		id: contactAdditionDialog

		ContactAdditionDialog {
			account: root.selectedAccount
		}
	}

	Component {
		id: contactAdditionPage

		ContactAdditionPage {
			account: root.selectedAccount
		}
	}

	Component {
		id: groupChatCreationDialog

		GroupChatCreationDialog {
			account: root.selectedAccount
		}
	}

	Component {
		id: groupChatCreationPage

		GroupChatCreationPage {
			account: root.selectedAccount
		}
	}

	Component {
		id: groupChatJoiningDialog

		GroupChatJoiningDialog {
			account: root.selectedAccount
		}
	}

	Component {
		id: groupChatJoiningPage

		GroupChatJoiningPage {
			account: root.selectedAccount
		}
	}

	Component {
		id: publicGroupChatSearchDialog

		PublicGroupChatSearchDialog {}
	}

	Component {
		id: aboutDialog

		AboutDialog {}
	}

	Component {
		id: aboutPage

		AboutPage {}
	}

	Connections {
		target: MainController

		function onOpenGlobalDrawerRequested() {
			root.open()
		}

		function onUserJidReceived(userJid) {
			root.openContactAdditionView().jid = userJid
		}
	}

	Connections {
		target: AccountController

		function onNoAccountAvailable() {
			root.close()
		}
	}

	// Needs to be outside of the DetailsDialog to not be destroyed with it.
	// Otherwise, the undo action of "showPassiveNotification()" would point to a destroyed object.
	Connections {
		target: root.selectedAccount ? root.selectedAccount.blockingController : null

		function onUnblocked(jid) {
			// Show a passive notification when a JID that is not in the roster is unblocked and
			// provide an option to undo that.
			// JIDs in the roster can be blocked again via their details.
			if (!RosterModel.hasItem(root.selectedAccount.settings.jid, jid)) {
				showPassiveNotification(qsTr("Unblocked %1").arg(jid), "long", qsTr("Undo"), () => {
					root.selectedAccount.blockingController.block(jid)
				})
			}
		}

		function onBlockingFailed(jid, errorText) {
			showPassiveNotification(qsTr("Could not block %1: %2").arg(jid).arg(errorText))
		}

		function onUnblockingFailed(jid, errorText) {
			showPassiveNotification(qsTr("Could not unblock %1: %2").arg(jid).arg(errorText))
		}
	}

	function openContactAdditionView() {
		openViewFromGlobalDrawer(contactAdditionDialog, contactAdditionPage)
	}

	function openViewFromGlobalDrawer(overlayComponent, pageComponent) {
		if (Kirigami.Settings.isMobile) {
			openPageFromGlobalDrawer(pageComponent)
		} else {
			openOverlayFromGlobalDrawer(overlayComponent)
		}
	}

	function openOverlayFromGlobalDrawer(overlayComponent) {
		nextOverlayToOpen = overlayComponent
		selectAccount()
	}

	function openPageFromGlobalDrawer(pageComponent) {
		nextPageToOpen = pageComponent
		selectAccount()
	}

	function selectAccount() {
		if (enabledAccountCount === 1) {
			for (let i in accounts) {
				const account = accounts[i]

				if (account.settings.enabled) {
					selectedAccount = account
					break
				}
			}
		} else {
			openOverlay(accountSelectionDialog)
		}

		close()
	}
}
