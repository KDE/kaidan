// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2018 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Bhavy Airi <airiraghav@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigami 2.19 as Kirigami
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

import im.kaidan.kaidan 1.0

import "elements"
import "details"

Kirigami.GlobalDrawer {
	id: root

	property string selectedAccountJid

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

					ListView {
						model: [ AccountManager.jid ]
						delegate: ColumnLayout {
							spacing: 0
							width: ListView.view.width

							MobileForm.FormTextDelegate {
								id: accountArea

								property bool disconnected: Kaidan.connectionState === Enums.StateDisconnected
								property bool connected: Kaidan.connectionState === Enums.StateConnected

								background: MobileForm.FormDelegateBackground { control: accountArea }
								leading: Avatar {
									jid: modelData
									name: AccountManager.displayName
								}
								leadingPadding: 10
								// The placeholder text is used while "AccountManager.displayName"
								// is not yet loaded to avoid a binding loop for the property
								// "implicitHeight".
								text: AccountManager.displayName ? AccountManager.displayName : " "
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
									root.selectedAccountJid = modelData
									openViewFromGlobalDrawer(accountDetailsDialog, accountDetailsPage).jid = modelData
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
						onClicked: openPageFromGlobalDrawer(contactAdditionQrCodePage)
					}

					MobileForm.FormButtonDelegate {
						text: qsTr("Add contact by chat address")
						icon.name: "contact-new-symbolic"
						onClicked: openContactAdditionView()
					}

					MobileForm.FormButtonDelegate {
						text: qsTr("Create group")
						icon.name: "resource-group-new"
						visible: Kaidan.connectionState === Enums.StateConnected && GroupChatController.groupChatCreationSupported
						onClicked: openViewFromGlobalDrawer(groupChatCreationDialog, groupChatCreationPage)
					}

					MobileForm.FormButtonDelegate {
						text: qsTr("Join group")
						icon.name: "resource-group"
						visible: Kaidan.connectionState === Enums.StateConnected && GroupChatController.groupChatParticipationSupported
						onClicked: openViewFromGlobalDrawer(groupChatJoiningDialog, groupChatJoiningPage)
					}

					MobileForm.FormButtonDelegate {
						id: publicGroupChatSearchButton
						text: qsTr("Search public groups")
						icon.name: "system-search-symbolic"
						onClicked: openOverlayFromGlobalDrawe(searchPublicGroupChatDialog)

						Shortcut {
							sequence: "Ctrl+G"
							onActivated: publicGroupChatSearchButton.clicked()
						}
					}

					MobileForm.FormButtonDelegate {
						text: qsTr("Switch device")
						icon.name: "send-to-symbolic"
						onClicked: openPageFromGlobalDrawer(deviceSwitchingPage)
					}

					MobileForm.FormButtonDelegate {
						text: qsTr("About")
						icon.name: "help-about-symbolic"
						onClicked: openViewFromGlobalDrawer(aboutDialog, aboutPage)
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
			accountJid: AccountManager.jid
			nickname: AccountManager.displayName
		}
	}

	Component {
		id: groupChatCreationPage

		GroupChatCreationPage {
			accountJid: AccountManager.jid
			nickname: AccountManager.displayName
		}
	}

	Component {
		id: searchPublicGroupChatDialog

		SearchPublicGroupChatDialog {}
	}

	Component {
		id: deviceSwitchingPage

		DeviceSwitchingPage {}
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
		root.close()
		return openOverlay(overlayComponent)
	}

	function openPageFromGlobalDrawer(pageComponent) {
		root.close()
		return openPage(pageComponent)
	}

	function openViewFromGlobalDrawer(overlayComponent, pageComponent) {
		root.close()
		return openView(overlayComponent, pageComponent)
	}

	Connections {
		target: Kaidan

		function onCredentialsNeeded() {
			close()
		}

		function onXmppUriReceived(uriString) {
			const xmppUriPrefix = `xmpp:`
			openContactAdditionView().jid = uri.substr(xmppUriPrefix.length)
		}
	}
}
