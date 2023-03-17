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
import org.kde.kirigami 2.12 as Kirigami
import "../elements"
import im.kaidan.kaidan 1.0
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

SettingsPageBase {
	title: qsTr("Change password")

	implicitHeight: layout.implicitHeight
	implicitWidth: layout.implicitWidth

	ColumnLayout {
		id: layout
		anchors.fill: parent

		Layout.preferredWidth: 600
		visible: !busyIndicator.visible

		Controls.BusyIndicator {
			id: busyIndicator
			visible: false
			Layout.alignment: Qt.AlignCenter
			width: 60
			height: 60
		}

		MobileForm.FormCard {
			Layout.fillWidth: true

			contentItem: ColumnLayout {
				spacing: 0

				MobileForm.FormCardHeader {
					title: qsTr("Information")
				}
				MobileForm.AbstractFormDelegate {
					Layout.fillWidth: true
					contentItem: RowLayout {
						Kirigami.Icon {
							source: "documentinfo"
						}
						Controls.Label {
							Layout.fillWidth: true
							text: qsTr("After changing your password, you will need to reenter it on all your other devices.")
							wrapMode: Text.Wrap
						}
					}
				}
			}
		}
		MobileForm.FormCard {
			Layout.fillWidth: true

			contentItem: ColumnLayout {
				MobileForm.AbstractFormDelegate {
					Layout.fillWidth: true
					visible: Kaidan.settings.passwordVisibility === Kaidan.PasswordInvisible
					contentItem: ColumnLayout {
						Controls.Label {
							text: qsTr("Current password:")
						}

						Controls.TextField {
							id: oldPassword
							Layout.fillWidth: true
							echoMode: TextInput.Password
							selectByMouse: true
						}
					}
				}
				MobileForm.AbstractFormDelegate {
					Layout.fillWidth: true
					contentItem: ColumnLayout {
						Controls.Label {
							text: qsTr("New password:")
						}

						Controls.TextField {
							id: password1
							Layout.fillWidth: true
							echoMode: TextInput.Password
							selectByMouse: true
						}
					}
				}
				MobileForm.AbstractFormDelegate {
					Layout.fillWidth: true
					contentItem: ColumnLayout {
						Controls.Label {
							text: qsTr("New password (repeat):")
						}

						Controls.TextField {
							id: password2
							Layout.fillWidth: true
							echoMode: TextInput.Password
							selectByMouse: true
						}
					}
				}
				Controls.Control {
					padding: Kirigami.Units.smallSpacing
					visible: password1.text !== password2.text
					Layout.fillWidth: true

					contentItem: Kirigami.InlineMessage {
						visible: password1.text !== password2.text
						type: Kirigami.MessageType.Warning
						text: qsTr("New passwords do not match.")
						showCloseButton: true
					}
				}
				Controls.Control {
					padding: Kirigami.Units.smallSpacing
					visible: false
					id: currentPasswordInvalidMessage
					Layout.fillWidth: true

					contentItem: Kirigami.InlineMessage {
						type: Kirigami.MessageType.Warning
						text: qsTr("Current password is invalid.")
						showCloseButton: true
					}
				}
			}
		}

		Item {
			Layout.fillHeight: true
		}

		MobileForm.FormCard {
			id: card
			Layout.fillWidth: true

			contentItem: RowLayout {
				spacing: 0
				MobileForm.AbstractFormDelegate {
					Layout.fillWidth: true
					implicitWidth: (card.width / 2) - 1
					onClicked: stack.pop()
					contentItem: RowLayout {
						Kirigami.Icon {
							source: "dialog-cancel"
						}
						Controls.Label {
							Layout.fillWidth: true
							text: qsTr("Cancel")
							wrapMode: Text.Wrap
						}
					}
				}

				Kirigami.Separator {
					Layout.fillHeight: true
				}
				MobileForm.AbstractFormDelegate {
					opacity: password1.text === password2.text ? 1:0.3
					Layout.fillWidth: true

					implicitWidth: (card.width / 2) - 1
					onClicked: {
						if (!enabled) {
							return
						}

						if (oldPassword.visible) {
							if (oldPassword.text !== AccountManager.password) {
								currentPasswordInvalidMessage.visible = true
								return
							}

							currentPasswordInvalidMessage.visible = false
						}

						Kaidan.client.registrationManager.changePasswordRequested(
									password1.text)
						busyIndicator.visible = true
					}
					enabled: password1.text === password2.text
					contentItem: RowLayout {
						Kirigami.Icon {
							source: "checkbox"
						}
						Controls.Label {
							Layout.fillWidth: true
							text: qsTr("Change")
							wrapMode: Text.Wrap
						}
					}
				}
			}
		}
	}

	Connections {
		target: Kaidan

		function onPasswordChangeSucceeded() {
			busyIndicator.visible = false
			passiveNotification(qsTr("Password changed successfully"))
			stack.pop()
		}

		function onPasswordChangeFailed(errorMessage) {
			busyIndicator.visible = false
			passiveNotification(qsTr("Failed to change password: %1").arg(errorMessage))
		}
	}
}
