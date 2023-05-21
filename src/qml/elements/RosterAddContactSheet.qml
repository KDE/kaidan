// SPDX-FileCopyrightText: 2017 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2017 Ilya Bizyaev <bizyaev@zoho.com>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Mathis Brüchert <mbblp@protonmail.ch>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
// SPDX-FileCopyrightText: 2023 Mathis Brüchert <mbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
