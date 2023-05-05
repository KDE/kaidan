/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2023 Kaidan developers and contributors
 *  (see the LICENSE file for a full list of copyright authors)
 *
 *  Kaidan is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  In addition, as a special exception, the author of Kaidan gives
 *  permission to link the code of its release with the OpenSSL
 *  project's "OpenSSL" library (or with modified versions of it that
 *  use the same license as the "OpenSSL" library), and distribute the
 *  linked executables. You must obey the GNU General Public License in
 *  all respects for all of the code used other than "OpenSSL". If you
 *  modify this file, you may extend this exception to your version of
 *  the file, but you are not obligated to do so.  If you do not wish to
 *  do so, delete this exception statement from your version.
 *
 *  Kaidan is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kaidan.  If not, see <http://www.gnu.org/licenses/>.
 */

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

	AccountDetailsSheet {
		id: accountDetailsSheet
	}

	Component {
		id: accountDetailsPage

		AccountDetailsPage {}
	}

	RosterAddContactSheet {
		id: contactAdditionSheet
		jid: ""
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
									if (Kirigami.Settings.isMobile) {
										if (pageStack.layers.depth < 2) {
											pageStack.layers.push(accountDetailsPage)
											root.close()
										}
									} else {
										root.close()
										accountDetailsSheet.open()
									}
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
						onClicked: {
							root.close()
							contactAdditionSheet.open()
						}
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

	Connections {
		target: Kaidan

		function onCredentialsNeeded() {
			accountDetailsSheet.close()
			close()
		}

		function onXmppUriReceived(uri) {
			// 'xmpp:' has length 5.
			contactAdditionSheet.jid = uri.substr(5)
			contactAdditionSheet.open()
		}
	}
}
