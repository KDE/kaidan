// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2018 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Bhavy Airi <airiraghav@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls
import org.kde.kirigami 2.19 as Kirigami
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

import im.kaidan.kaidan 1.0

import "elements"
import "details"
import "settings"

Kirigami.GlobalDrawer {
	id: root

	Component {
		id: qrCodePage

		QrCodePage {}
	}

	Component {
		id: accountDetailsSheet

		AccountDetailsSheet {}
	}

	Component {
		id: accountDetailsPage

		AccountDetailsPage {}
	}

	SearchPublicGroupChatSheet {
		id: searchPublicGroupChatSheet
	}

	SettingsSheet {
		id: settingsSheet
	}

	topContent: [
		ColumnLayout {
			spacing: Kirigami.Units.largeSpacing
			Layout.margins: -3

			MobileForm.FormCard {
				Layout.fillWidth: true

				contentItem: ColumnLayout {
					spacing: 0

					MobileForm.FormCardHeader {
						title: qsTr("Accounts")
					}

					Repeater {
						model: [ AccountManager.jid ]

						delegate: ColumnLayout {
							spacing: 0

							MobileForm.FormTextDelegate {
								id: accountArea

								property bool disconnected: Kaidan.connectionState === Enums.StateDisconnected
								property bool connected: Kaidan.connectionState === Enums.StateConnected

								background: MobileForm.FormDelegateBackground { control: accountArea }
								leftPadding: 15
								leading: Avatar {
									jid: AccountManager.jid
									name: AccountManager.displayName
								}
								leadingPadding: 10
								text: AccountManager.displayName
								description: {
									const color = connected ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.disabledTextColor
									return "<font color='" + color + "'>" + Kaidan.connectionStateText + "</font>"
								}
								trailing: Controls.Switch {
									checked: !accountArea.disconnected
									onToggled: accountArea.disconnected ? Kaidan.logIn() : Kaidan.logOut()
								}
								onClicked: {
									root.close()
									openViewFromGlobalDrawer(accountDetailsSheet, accountDetailsPage)
								}
							}

							Controls.Label {
								id: errorMessage
								visible: Kaidan.connectionError
								text: Kaidan.connectionError ? Utils.connectionErrorMessage(Kaidan.connectionError) : ""
								font.bold: true
								wrapMode: Text.WordWrap
								padding: 10
								Layout.margins: 10
								Layout.fillWidth: true
								background: Rectangle {
									color: Kirigami.Theme.negativeBackgroundColor
									radius: roundedCornersRadius
								}
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
						title: qsTr("Actions")
					}

					MobileForm.FormButtonDelegate {
						text: qsTr("Add contact by QR code")
						icon.name: "view-barcode-qr"
						onClicked: {
							root.close()
							pageStack.layers.push(qrCodePage)
						}
					}

					MobileForm.FormButtonDelegate {
						text: qsTr("Add contact by chat address")
						icon.name: "contact-new-symbolic"
						onClicked: openContactAdditionView()
					}

					MobileForm.FormButtonDelegate {
						id: publicGroupChatSearchButton
						text: qsTr("Search public groups")
						icon.name: "system-search-symbolic"
						onClicked: {
							root.close()
							searchPublicGroupChatSheet.open()
						}

						Shortcut {
							sequence: "Ctrl+G"
							onActivated: publicGroupChatSearchButton.clicked()
						}
					}

					MobileForm.FormButtonDelegate {
						text: qsTr("Invite friends")
						icon.name: "mail-message-new-symbolic"
						onClicked: {
							Utils.copyToClipboard(Utils.invitationUrl(AccountManager.jid))
							passiveNotification(qsTr("Invitation link copied to clipboard"))
						}
					}

					MobileForm.FormButtonDelegate {
						text: qsTr("Switch device")
						icon.name: "send-to-symbolic"
						onClicked: {
							root.close()
							pageStack.layers.push("AccountTransferPage.qml")
						}
					}

					MobileForm.FormButtonDelegate {
						text: qsTr("Settings")
						icon.name: "preferences-system-symbolic"
						onClicked: {
							root.close()

							if (Kirigami.Settings.isMobile) {
								if (pageStack.layers.depth < 2)
									pageStack.layers.push(settingsPage)
							} else {
								settingsSheet.open()
							}
						}
					}
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
			Kaidan.client.vCardManager.clientVCardRequested()
		}

		// Retrieve the user's own OMEMO key to be used while adding a contact via QR code.
		// That is only done when no chat is already open.
		// Otherwise, it would result in an unneccessary fetching and it would remove the cached
		// keys for that chat while only keeping the own key in the cache.
		if (!MessageModel.currentChatJid.length) {
			Kaidan.client.omemoManager.retrieveOwnKeyRequested()
		}
	}

	function openContactAdditionView() {
		return openViewFromGlobalDrawer(contactAdditionDialog, contactAdditionPage)
	}

	function openViewFromGlobalDrawer(overlayComponent, pageComponent) {
		root.close()
		return openView(overlayComponent, pageComponent)
	}

	Connections {
		target: Kaidan

		function onCredentialsNeeded() {
			accountDetailsSheet.close()
			close()
		}

		function onXmppUriReceived(uri) {
			const xmppUriPrefix = `xmpp:`
			openContactAdditionView().jid = uri.substr(xmppUriPrefix.length)
		}
	}
}
