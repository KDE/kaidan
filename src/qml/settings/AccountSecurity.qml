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
	title: qsTr("Account Security")

	implicitHeight: layout.implicitHeight
	implicitWidth: layout.implicitWidth
	ColumnLayout {
		id: layout

		anchors.fill: parent
		Layout.preferredWidth: 600
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
							Layout.rightMargin: (root.icon.name !== "") ? Kirigami.Units.largeSpacing : 0
							implicitWidth: (root.icon.name !== "") ? Kirigami.Units.iconSizes.small : 0
							implicitHeight: (root.icon.name !== "") ? Kirigami.Units.iconSizes.small : 0
						}
						Controls.Label {
							Layout.fillWidth: true
							text: qsTr("Configure password visibility for switching devices, by default your password can be shown as text and in form of a QR code. You can change this here.")
							elide: Text.ElideRight
							wrapMode: Text.Wrap
						}
					}
				}
			}
		}
		MobileForm.FormCard {
			Layout.fillWidth: true

			contentItem: ColumnLayout {
				spacing: 0

				MobileForm.FormButtonDelegate {
					visible: Kaidan.serverFeaturesCache.inBandRegistrationSupported
					text: qsTr("Don't show password")
					description: qsTr("Allow to add additional devices using the login QR code but never show the password.")
					onClicked: stack.push("qrc:/qml/settings/DisablePlainTextPasswordDisplay.qml")
					icon.name: "password-show-off"
				}
				MobileForm.FormButtonDelegate {
					visible: Kaidan.serverFeaturesCache.inBandRegistrationSupported
					text: qsTr("Don't expose password in any way")
					description: qsTr("Neither allow to add additional devices using the login QR code nor show the password.")
					onClicked: stack.push("qrc:/qml/settings/DisablePlainTextPasswordDisplay.qml")
					icon.name: "channel-secure-symbolic"
				}
			}
		}

		Item {
			Layout.fillHeight: true
		}
	}
}
