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
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

import "fields"

Kirigami.Dialog {
	property string jid: ""
	property string nickname: ""
	padding: Kirigami.Units.mediumSpacing
	parent: applicationWindow().overlay
	title: qsTr("Add new contact")
	standardButtons: Kirigami.Dialog.Cancel
	onRejected: {
		clearInput()
		close()
	}
	customFooterActions: [
		Kirigami.Action {
			text: qsTr("Add")
			enabled: credentialsValidator.isAccountJidValid(jidField.text)
			iconName: "list-add"
			onTriggered: {
				Kaidan.client.rosterManager.addContactRequested(
						jidField.text.toLowerCase(),
						nickField.text,
						msgField.text
				)
				clearInput()
				close()

				Kaidan.openChatPageRequested(AccountManager.jid, jidField.text)
			}
		}
	]

	CredentialsValidator {
		id: credentialsValidator
	}

	ColumnLayout {
		Layout.fillWidth: true
		Kirigami.InlineMessage {
			visible: true
			Layout.preferredWidth: 400
			Layout.fillWidth: true
			type: Kirigami.MessageType.Information

			text:  qsTr("This will also send a request to access the " +
						"presence of the contact.")
		}

		Controls.Label {
			text: qsTr("Jabber-ID:")
		}
		JidField {
			id: jidField
			text: jid
			Layout.fillWidth: true
		}

		Controls.Label {
			text: qsTr("Nickname:")
		}
		Controls.TextField {
			id: nickField
			selectByMouse: true
			Layout.fillWidth: true
			text: nickname
		}

		Controls.Label {
			text: qsTr("Optional message:")
			textFormat: Text.PlainText
			Layout.fillWidth: true
		}
		Controls.TextArea {
			id: msgField
			Layout.fillWidth: true
			Layout.minimumHeight: Kirigami.Units.gridUnit * 4
			placeholderText: qsTr("Tell your chat partner who you are.")
			wrapMode: Controls.TextArea.Wrap
			selectByMouse: true
		}
	}

	function clearInput() {
		jid = "";
		nickname = "";
		msgField.text = "";
	}
}
