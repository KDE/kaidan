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
import im.kaidan.kaidan 1.0
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

/**
 * The settings page contains options to configure Kaidan.
 *
 * It is used on a new layer on mobile and inside of a Sheet on desktop.
 */
ColumnLayout {
	property string title: qsTr("Settings")

	Layout.fillHeight: true

	MobileForm.FormCard {
		Layout.fillWidth: true
		Layout.fillHeight: true

		contentItem: ColumnLayout {
			spacing: 0


			MobileForm.FormCardHeader {
				title: qsTr("Account Settings")
			}

			MobileForm.FormButtonDelegate {
				text: qsTr("Account")
				description: qsTr("Edit your profile")
				onClicked: stack.push("qrc:/qml/settings/AccountSettings.qml")
				icon.name: "avatar-default-symbolic"
			}

			MobileForm.FormButtonDelegate {
				visible: Kaidan.serverFeaturesCache.inBandRegistrationSupported
				text: qsTr("Change password")
				description: qsTr("Change your account's password")
				onClicked: stack.push("qrc:/qml/settings/ChangePassword.qml")
				icon.name: "system-lock-screen-symbolic"
			}

			MobileForm.FormButtonDelegate {
				text: qsTr("Account security")
				description: qsTr("Configure whether this device can be used to switch to another device")
				icon.name: "security-high-symbolic"
				onClicked: stack.push("qrc:/qml/settings/AccountSecurity.qml")
			}
			MobileForm.FormButtonDelegate {
				text: qsTr("Log out")
				description: qsTr("Log out account from this app")
				icon.name: "system-log-out"
				onClicked: stack.push("qrc:/qml/settings/LocalAccountRemoval.qml")
			}
			MobileForm.FormButtonDelegate {
				text: qsTr("Delete account")
				description: qsTr("Delete account from the server")
				icon.name: "edit-delete-symbolic"
				onClicked: stack.push("qrc:/qml/settings/RemoteAccountDeletion.qml")
			}
		}
	}
	MobileForm.FormCard {
		Layout.fillWidth: true
		contentItem: ColumnLayout {
			spacing: 0
			MobileForm.FormCardHeader {
				title: qsTr("Application Settings")
			}
			MobileForm.FormButtonDelegate {
				text: qsTr("Multimedia Settings")
				description: qsTr("Configure photo, video and audio recording settings")
				onClicked: stack.push("qrc:/qml/settings/MultimediaSettings.qml")
				icon.name: "emblem-system-symbolic"
			}
			MobileForm.FormButtonDelegate {
				text: qsTr("Connection Settings")
				description: qsTr("Configure the hostname and port to connect to")
				onClicked: stack.push("qrc:/qml/settings/ConnectionSettings.qml")
				icon.name: "settings-configure"
			}
		}
	}
	MobileForm.FormCard {
		Layout.fillWidth: true
		contentItem: ColumnLayout {
			spacing: 0
			MobileForm.FormCardHeader {
				title: qsTr("About")
			}
			MobileForm.FormButtonDelegate {
				text: qsTr("About Kaidan")
				description: qsTr("Learn about the current Kaidan version, view the source code and contribute")
				onClicked: stack.push("qrc:/qml/settings/AboutPage.qml")
				icon.name: "help-about-symbolic"
			}

		}
	}
}
