/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2022 Kaidan developers and contributors
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

import im.kaidan.kaidan 1.0
import "elements"

Kirigami.Page {
	id: root
	title: qsTr("Profile")
	topPadding: 0
	rightPadding: 0
	bottomPadding: 0
	leftPadding: 0

	required property string jid
	required property RosterItemWatcher chatItemWatcher

	Timer {
		id: pageTimer
		interval: 10

		onTriggered: {
			if (!root.isCurrentPage) {
				// Close the current page if it's not the current one after 10ms
				pageStack.pop();
			}

			// Stop the timer regardless of whether the page was closed or not
			pageTimer.stop();
		}
	}

	onIsCurrentPageChanged: {
		/*
		 * Start the timer if we are getting or loosing focus.
		 * Probably due to some kind of animation, isCurrentPage changes a few ms after
		 * this has been triggered.
		 */
		pageTimer.start();
	}

	actions {
		left: Kirigami.Action {
			text: qsTr("Rename contact")
			icon.name: "document-edit-symbolic"

			onTriggered: {
				renameSheet.open()
				renameSheet.forceActiveFocus()
			}
		}

		main: Kirigami.Action {
			text: qsTr("Show QR code")
			icon.name: "view-barcode-qr"
			onTriggered: qrCodeSheet.open()
		}

		right: Kirigami.Action {
			text: qsTr("Remove contact")
			icon.name: "edit-delete-symbolic"
			onTriggered: removeSheet.open()
		}
	}

	UserResourcesWatcher {
		id: ownResourcesWatcher
		jid: MessageModel.currentAccountJid
	}

	UserPresenceWatcher {
		id: userPresence
		jid: root.jid
	}

	RosterRenameContactSheet {
		id: renameSheet
		jid: root.jid
		enteredName: chatItemWatcher.item.name
	}

	Kirigami.OverlaySheet {
		id: qrCodeSheet
		parent: applicationWindow().overlay

		ColumnLayout {
			QrCode {
				Layout.fillHeight: true
				Layout.fillWidth: true
				Layout.preferredWidth: 500
				Layout.preferredHeight: 500
				Layout.maximumHeight: applicationWindow().height * 0.5

				jid: root.jid
			}
		}
	}

	RosterRemoveContactSheet {
		id: removeSheet
		jid: root.jid
	}

	Controls.ScrollView {
		anchors.fill: parent
		clip: true
		contentWidth: root.width
		contentHeight: content.height

		ColumnLayout {
			id: content
			x: 20
			y: 5
			width: root.width - 40

			Item {
				Layout.preferredHeight: 10
			}

			RowLayout {
				Layout.alignment: Qt.AlignTop
				Layout.fillWidth: true
				spacing: 20

				Avatar {
					Layout.preferredHeight: Kirigami.Units.gridUnit * 10
					Layout.preferredWidth: Kirigami.Units.gridUnit * 10
					jid: root.jid
					name: chatItemWatcher.item.displayName
				}

				ColumnLayout {
					Kirigami.Heading {
						Layout.fillWidth: true
						text: chatItemWatcher.item.displayName
						textFormat: Text.PlainText
						maximumLineCount: 2
						elide: Text.ElideRight
					}

					Controls.Label {
						text: root.jid
						color: Kirigami.Theme.disabledTextColor
						textFormat: Text.PlainText
					}

					RowLayout {
						spacing: Kirigami.Units.smallSpacing

						Kirigami.Icon {
							source: userPresence.availabilityIcon
							width: 26
							height: 26
						}

						Controls.Label {
							Layout.alignment: Qt.AlignVCenter
							text: userPresence.availabilityText
							color: userPresence.availabilityColor
							textFormat: Text.PlainText
						}

						Item {
							Layout.fillWidth: true
						}
					}
				}
			}

			Item {
				height: Kirigami.Units.largeSpacing
			}

			Kirigami.Heading {
				level: 2
				text: qsTr("Profile")
			}

			Repeater {
				model: VCardModel {
					jid: root.jid
				}

				delegate: ColumnLayout {
					Layout.fillWidth: true

					Controls.Label {
						text: Utils.formatMessage(model.value)
						onLinkActivated: Qt.openUrlExternally(link)
						textFormat: Text.StyledText
					}

					Controls.Label {
						text: model.key
						color: Kirigami.Theme.disabledTextColor
						textFormat: Text.PlainText
					}

					Item {
						height: 3
					}
				}
			}

			Item {
				height: Kirigami.Units.largeSpacing
			}

			RowLayout {
				Kirigami.Heading {
					level: 2
					text: qsTr("Encryption (OMEMO 2)")
				}

				Controls.Switch {
					id: omemoEncryptionSwitch
					enabled: MessageModel.usableOmemoDevices.length
					checked: MessageModel.isOmemoEncryptionEnabled
					onClicked: {
						// The switch is toggled by setting the user's preference on using encryption.
						// Note that 'checked' has already the value after the button is clicked.
						if (checked) {
							MessageModel.encryption = Encryption.Omemo2
						} else {
							MessageModel.encryption = Encryption.NoEncryption
						}
					}

					Layout.fillWidth: true
				}
			}

			ColumnLayout {
				Layout.maximumWidth: largeButtonWidth
				spacing: Kirigami.Units.largeSpacing

				Controls.Label {
					text: qsTr("End-to-end encryption with OMEMO 2 ensures that nobody else than you and your chat partners can read or modify the data you exchange.")
					color: Kirigami.Theme.disabledTextColor
					wrapMode: Text.WordWrap
					Layout.fillWidth: true
				}

				ColumnLayout {
					Controls.Label {
						id: ownOmemoDevicesExplanation
						text: {
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

						visible: text
						wrapMode: Text.WordWrap
					}

					CenteredAdaptiveButton {
						text: qsTr("Scan own QR codes")
						icon.name: "view-barcode-qr"
						visible: MessageModel.ownAuthenticatableOmemoDevices.length
						onClicked: pageStack.layers.push(qrCodePage, { isForOwnDevices: true })
					}
				}

				ColumnLayout {
					Controls.Label {
						id: contactOmemoDevicesExplanation
						text: {
							if (!MessageModel.usableOmemoDevices.length) {
								if (MessageModel.distrustedOmemoDevices.length) {
									return qsTr("Scan the QR code of your contact to enable encryption")
								}

								return qsTr("Your contact doesn't use OMEMO 2")
							} else if (MessageModel.authenticatableOmemoDevices.length) {
								if (MessageModel.authenticatableOmemoDevices.length === MessageModel.distrustedOmemoDevices.length) {
									return qsTr("Scan the QR codes of your contact's devices to encrypt for them")
								}

								return qsTr("Scan the QR code of your contact for maximum secure encryption")
							}

							return ""
						}

						visible: text
						wrapMode: Text.WordWrap
					}

					CenteredAdaptiveButton {
						text: qsTr("Scan contact's QR code")
						icon.name: "view-barcode-qr"
						visible: MessageModel.authenticatableOmemoDevices.length
						onClicked: pageStack.layers.push(qrCodePage, { contactJid: root.jid })
					}
				}
			}

			Item {
				height: Kirigami.Units.largeSpacing
			}

			Kirigami.Heading {
				level: 2
				text: qsTr("Online devices")
			}

			Repeater {
				model: UserDevicesModel {
					jid: root.jid
				}

				delegate: ColumnLayout {
					Controls.Label {
						text: {
							if (model.name) {
								var result = model.name
								if (model.version) {
									result += " v" + model.version
								}
								if (model.os) {
									result += " • " + model.os
								}
								return result
							}

							return model.resource
						}
						textFormat: Text.PlainText
					}
					Item {
						height: 3
					}
				}
			}

			// placeholder for left, right and main action
			Item {
				visible: Kirigami.Settings.isMobile
				Layout.preferredHeight: 60
			}
		}
	}
}
