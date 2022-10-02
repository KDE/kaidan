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
import org.kde.kirigami 2.12 as Kirigami
import im.kaidan.kaidan 1.0

/**
 * The settings page contains options to configure Kaidan.
 *
 * It is used on a new layer on mobile and inside of a Sheet on desktop.
 */
ColumnLayout {
	property string title: qsTr("Settings")
	spacing: 0
	height: applicationWindow().height * 0.9

	SettingsItem {
		name: qsTr("Change password")
		description: qsTr("Changes your account's password. You will need to re-enter it on your other devices.")
		visible: Kaidan.serverFeaturesCache.inBandRegistrationSupported
		onClicked: stack.push("ChangePassword.qml")
		icon: "system-lock-screen-symbolic"
	}
	SettingsItem {
		name: qsTr("Multimedia Settings")
		description: qsTr("Configure photo, video and audio recording settings")
		onClicked: stack.push("MultimediaSettings.qml")
		icon: "emblem-system-symbolic"
	}
	SettingsItem {
		name: qsTr("Connection Settings")
		description: qsTr("Configure the hostname and port to connect to")
		onClicked: stack.push("ConnectionSettings.qml")
		icon: "settings-configure"
	}
	SettingsItem {
		name: qsTr("Account security")
		description: qsTr("Configure whether this device can be used to log in on another device")
		icon: "security-high-symbolic"
		onClicked: stack.push("AccountSecurity.qml")
	}
	SettingsItem {
		name: qsTr("Remove account from Kaidan")
		description: qsTr("Remove account from this app")
		icon: "system-log-out"
		onClicked: stack.push("LocalAccountRemoval.qml")
	}
	SettingsItem {
		name: qsTr("Delete account")
		description: qsTr("Delete account from the server")
		icon: "edit-delete-symbolic"
		onClicked: stack.push("RemoteAccountDeletion.qml")
	}
	Item {
		Layout.fillHeight: true
	}
}
