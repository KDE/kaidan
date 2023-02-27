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
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import QtQml 2.14
import org.kde.kirigami 2.12 as Kirigami
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

import im.kaidan.kaidan 1.0
import "elements"

Kirigami.OverlaySheet {
	id: root
	required property string jid
	required property RosterItemWatcher chatItem
	Kirigami.Theme.inherit: false
	Kirigami.Theme.colorSet: Kirigami.Theme.Window

	parent: applicationWindow().overlay

	header: RowLayout {
		UserPresenceWatcher {
			id: userPresence
			jid: root.jid
		}
		UserResourcesWatcher {
			id: ownResourcesWatcher
			jid: AccountManager.jid
		}

		Layout.preferredWidth: 300

		id: headerLayout
		Avatar {
			Rectangle {
				Rectangle {
					color: userPresence.availabilityColor
					radius: 50
					height: 25
					width: 25
					x: (parent.width - width) / 2
					y: (parent.height - height) / 2
				}
				color: Kirigami.Theme.backgroundColor
				radius: 50
				height: 35
				width: 35
				x: Kirigami.Units.gridUnit * 5
				y: Kirigami.Units.gridUnit * 5
				visible: userPresence.availability !== Presence.Offline
			}
			Layout.margins: 10
			implicitHeight: Kirigami.Units.gridUnit * 7
			implicitWidth: Kirigami.Units.gridUnit * 7
			jid: root.jid
			name: chatItem.item.displayName
		}

		ColumnLayout {
			Layout.alignment: Qt.AlignLeft
			Kirigami.Heading {
				text: chatItem.item.displayName
			}
			Controls.Label {
				text: root.jid
				color: Kirigami.Theme.disabledTextColor
			}
		}
		Item {
			Layout.fillWidth: true
		}
		Controls.RoundButton {
			id: qrButton
			checkable: true
			implicitHeight: 70
			implicitWidth: 70
			icon.name: qrButton.checked ? "draw-arrow-back" : "view-barcode-qr"

			Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
		}
		Controls.ToolButton {
			RosterRenameContactSheet {
				id: renameSheet
				jid: root.jid
				enteredName: chatItemWatcher.item.name
			}

			RosterRemoveContactSheet {
				id: removeSheet
				jid: root.jid
			}
			Layout.alignment: Qt.AlignTop
			id: actionButton
			icon.name: "overflow-menu"
			onClicked: menu.open()

			Controls.Menu {
				id: menu
				y: actionButton.height
				z: 100000

				Controls.MenuItem {
					text: qsTr("Rename contact")
					icon.name: "document-edit-symbolic"

					onTriggered: {
						renameSheet.open()
						renameSheet.forceActiveFocus()
					}
				}
				Controls.MenuItem {
					text: qsTr("Remove contact")
					icon.name: "albumfolder-user-trash"
					onTriggered: removeSheet.open()
				}
			}
		}
	}

	contentItem: Controls.Control {
		id: control
		leftPadding: 0
		rightPadding: 0
		topPadding: Kirigami.Units.gridUnit
		bottomPadding: Kirigami.Units.gridUnit

		background: Rectangle {
			Kirigami.Theme.inherit: false
			Kirigami.Theme.colorSet: Kirigami.Theme.Window
			color: Kirigami.Theme.backgroundColor
		}

		contentItem: Loader {
			Component {
				id: formComponent

				ColumnLayout {
					id: form
					implicitWidth: 500

					MobileForm.FormCard {
						Layout.fillWidth: true
						Layout.topMargin: Kirigami.Units.largeSpacing

						contentItem: ColumnLayout {
							spacing: 0
							MobileForm.FormCardHeader {
								title: qsTr("Profile Information")
							}

							MobileForm.AbstractFormDelegate {
								visible: infoRepeater.count == 0
								Layout.fillWidth: true
								contentItem: Kirigami.PlaceholderMessage {
									icon.name: "help-hint"
									text: qsTr("No information available")
								}
							}

							Repeater {
								id: infoRepeater
								Layout.fillHeight: true

								model: VCardModel {
									jid: root.jid
								}
								delegate: MobileForm.AbstractFormDelegate {
									Layout.fillWidth: true
									contentItem: ColumnLayout {
										Controls.Label {
											text: Utils.formatMessage(
													  model.value)
											onLinkActivated: Qt.openUrlExternally(
																 link)
											textFormat: Text.StyledText
										}
										Controls.Label {
											text: model.key
											color: Kirigami.Theme.disabledTextColor
											textFormat: Text.PlainText
											font: Kirigami.Theme.smallFont
										}
									}
								}
							}
						}
					}
					MobileForm.FormCard {
						Layout.fillWidth: true
						Layout.topMargin: Kirigami.Units.largeSpacing

						contentItem: ColumnLayout {
							spacing: 0
							MobileForm.FormCardHeader {
								title: qsTr("Encryption")
							}
							MobileForm.FormSwitchDelegate {
								text: qsTr("Enable OMEMO 2 encryption")
								description: qsTr("End-to-end encryption with OMEMO 2 ensures that nobody else than you and your chat partners can read or modify the data you exchange.")
								enabled: MessageModel.usableOmemoDevices.length
								checked: MessageModel.isOmemoEncryptionEnabled
								onClicked: {
									MessageModel.encryption = checked ? Encryption.Omemo2 : Encryption.NoEncryption
								}
							}

							MobileForm.FormButtonDelegate {
								visible: MessageModel.authenticatableOmemoDevices.length

								id: contactDeviceQrCodeScannerButton
								Layout.fillWidth: true
								icon.name: "camera-photo-symbolic"
								text: qsTr("Scan contact's QR code")
								description: qsTr("Scan the QR codes of your contact's devices to verify the encryption")

								onClicked: {
									pageStack.layers.push(qrCodePage, {
										"contactJid": root.jid
									})
									root.close()
								}

							}
							MobileForm.FormButtonDelegate {
								text: qsTr("Scan own QR codes")
								description: {
									if (!MessageModel.ownUsableOmemoDevices.length) {
										if (MessageModel.ownDistrustedOmemoDevices.length) {
											return qsTr("Scan the QR codes of your devices to encrypt for them")
										} else if (ownResourcesWatcher.resourcesCount > 1) {
											return qsTr("Your other devices don't use OMEMO 2")
										}
									} else if (MessageModel.ownAuthenticatableOmemoDevices.length) {
										if (MessageModel.ownAuthenticatableOmemoDevices.length === MessageModel.ownDistrustedOmemoDevices.length) {
											return qsTr("Scan the QR codes of your devices to encrypt for them")
										}

										return qsTr("Scan the QR codes of your devices for maximum secure encryption")
									}

									return ""
								}
								icon.name: "view-barcode-qr"
								visible: text
								onClicked: {
									pageStack.layers.push(qrCodePage, { isForOwnDevices: true })
									root.close()
								}
							}
						}
					}

					MobileForm.FormCard {
						Layout.fillWidth: true
						Layout.topMargin: Kirigami.Units.largeSpacing

						contentItem: ColumnLayout {
							spacing: 0

							MobileForm.FormCardHeader {
								title: qsTr("Privacy")
							}

							MobileForm.FormSwitchDelegate {
								text: qsTr("Send status")
								checked: chatItemWatcher.item.receivingPresence
								onClicked: {
									if (checked) {
										Kaidan.client.rosterManager.acceptSubscriptionToPresenceRequested(MessageModel.currentChatJid)
									} else {
										Kaidan.client.rosterManager.refuseSubscriptionToPresenceRequested(MessageModel.currentChatJid)
									}
								}
							}

							MobileForm.FormSwitchDelegate {
								text: qsTr("Send typing notifications")
								checked: chatItemWatcher.item.chatStateSendingEnabled
								onClicked: {
									RosterModel.setChatStateSendingEnabled(
										MessageModel.currentAccountJid,
										MessageModel.currentChatJid,
										checked)
								}
							}

							MobileForm.FormSwitchDelegate {
								text: qsTr("Send read notifications")
								checked: chatItemWatcher.item.readMarkerSendingEnabled
								onClicked: {
									RosterModel.setReadMarkerSendingEnabled(
										MessageModel.currentAccountJid,
										MessageModel.currentChatJid,
										checked)
								}
							}
						}
					}

					MobileForm.FormCard {
						Layout.fillWidth: true
						Layout.topMargin: Kirigami.Units.largeSpacing

						contentItem: ColumnLayout {
							spacing: 0

							MobileForm.FormCardHeader {
								title: qsTr("Devices")
							}
							MobileForm.AbstractFormDelegate {
								visible: deviceRepeater.count == 0
								Layout.fillWidth: true
								contentItem: Kirigami.PlaceholderMessage {
									icon.name: "network-offline"
									text: qsTr("No devices connected")
								}
							}

							Repeater {
								id: deviceRepeater
								Layout.fillHeight: true

								model: UserDevicesModel {
									jid: root.jid
								}
								delegate: MobileForm.AbstractFormDelegate {
									Layout.fillWidth: true
									contentItem: ColumnLayout {
										Controls.Label {
											text: {
												if (model.name) {
													if (model.version) {
														return model.name + " v" + model.version
													}
													return model.name
												}
												return model.resource
											}
										}
										Controls.Label {
											color: Kirigami.Theme.disabledTextColor
											text: model.os
											font: Kirigami.Theme.smallFont
										}
									}
								}
							}
						}
					}
				}
			}
			Component {
				id: qrComponent

				ColumnLayout {
					id: qr
					implicitWidth: 500

					QrCode {
						Layout.fillHeight: true
						Layout.fillWidth: true
						Layout.preferredWidth: 440
						Layout.preferredHeight: 440
						Layout.maximumHeight: qrCodeSheet.Width
						Layout.margins: 30
						jid: root.jid
					}
				}
			}
			sourceComponent: qrButton.checked ? qrComponent : formComponent
			active: true
		}
	}
}
