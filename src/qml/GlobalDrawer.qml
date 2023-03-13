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
import org.kde.kirigami 2.12 as Kirigami

import im.kaidan.kaidan 1.0

import "settings"

Kirigami.GlobalDrawer {
	id: globalDrawer

	SettingsSheet {
		id: settingsSheet
	}

	topContent: [
		// This item is used to disable an account temporarily.
		RowLayout {
			spacing: -4

			property bool disconnected: Kaidan.connectionState === Enums.StateDisconnected
			property bool connected: Kaidan.connectionState === Enums.StateConnected

			Controls.Switch {
				checked: !parent.disconnected
				onClicked: parent.disconnected ? Kaidan.logIn() : Kaidan.logOut()
			}

			Text {
				text: AccountManager.displayName + " (" + Kaidan.connectionStateText + ")"
				color: parent.connected ? "green" : "silver"
			}
		},

		Controls.Label {
			id: errorMessage
			visible: Kaidan.connectionError
			text: Kaidan.connectionError ? Utils.connectionErrorMessage(Kaidan.connectionError) : ""
			font.bold: true
			wrapMode: Text.WordWrap
			padding: 10
			Layout.leftMargin: 5
			Layout.rightMargin: 5
			Layout.fillWidth: true

			background: Rectangle {
				color: Kirigami.Theme.negativeBackgroundColor
				radius: roundedCornersRadius
			}
		}
	]

	actions: [
		Kirigami.Action {
			text: qsTr("Add contact by QR code")
			icon.name: "view-barcode-qr"
			onTriggered: pageStack.layers.push(qrCodePage)
		},
		Kirigami.Action {
			text: qsTr("Invite friends")
			icon.name: "mail-message-new-symbolic"
			onTriggered: {
				Utils.copyToClipboard(Utils.invitationUrl(AccountManager.jid))
				passiveNotification(qsTr("Invitation link copied to clipboard"))
			}
		},
		Kirigami.Action {
			text: qsTr("Switch device")
			icon.name: "send-to-symbolic"

			onTriggered: {
				pageStack.layers.push("AccountTransferPage.qml")
			}
		},
		Kirigami.Action {
			text: qsTr("Settings")
			icon.name: "preferences-system-symbolic"
			onTriggered: {
				// open settings page
				if (Kirigami.Settings.isMobile) {
					if (pageStack.layers.depth < 2)
						pageStack.layers.push(settingsPage)
				} else {
					settingsSheet.open()
				}
			}
		},
		Kirigami.Action {
			text: qsTr("About")
			icon.name: "help-about-symbolic"
			onTriggered: {
				popLayersAboveLowest()
				// open about sheet
				aboutDialog.open()
			}
		}
	]

	onOpened: {
		// Request the user's current vCard which contains the user's nickname.
		Kaidan.client.vCardManager.clientVCardRequested()

		// Retrieve the user's own OMEMO key to be used while adding a contact via QR code.
		Kaidan.client.omemoManager.retrieveOwnKeyRequested()
	}
}
