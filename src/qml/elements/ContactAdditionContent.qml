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

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

import "fields"

ConfirmationArea {
	id: root

	property Account account
	property alias jidField: jidField
	property alias jid: jidField.text
	property alias name: nameField.text

	confirmationButton.text: qsTr("Add")
	confirmationButton.onClicked: {
		if (RosterModel.hasItem(account.settings.jid, jid)) {
			showPassiveNotification(qsTr("Contact already exists"),
									"long",
									qsTr("Open chat"),
									function () {
										MainController.openChatPageRequested(account.settings.jid, jid)
									})
		} else if (jidField.valid) {
			busy = true
			account.rosterController.addContact(jid, name, messageField.text)
		} else {
			jidField.invalidHintMayBeShown = true
			jidField.forceActiveFocus()
		}
	}
	loadingArea.description: qsTr("Adding contact…")

	JidField {
		id: jidField
		Layout.fillWidth: true
		inputField.onAccepted: {
			if (valid) {
				nameField.forceActiveFocus()
			} else {
				invalidHintMayBeShown = true
				forceActiveFocus()
			}
		}
	}

	Field {
		id: nameField
		labelText: qsTr("Name (optional):")
		inputMethodHints: Qt.ImhPreferUppercase
		inputField.onAccepted: messageField.forceActiveFocus()
		Layout.fillWidth: true
	}

	ColumnLayout {
		Controls.Label {
			text: qsTr("Message (optional, unencrypted):")
			textFormat: Text.PlainText
			Layout.fillWidth: true
		}

		Controls.TextArea {
			id: messageField
			placeholderText: qsTr("Hello, I'm…")
			inputMethodHints: Qt.ImhPreferUppercase
			wrapMode: TextEdit.Wrap
			Keys.onPressed: (event) => {
				// If there is no message entered, add the contact by clicking the "Return" key.
				if (event.key === Qt.Key_Return && !text) {
					confirmationButton.clicked()
					event.accepted = true
				}
			}
			Layout.fillWidth: true
			Layout.minimumHeight: Kirigami.Units.gridUnit * 4
		}
	}

	Connections {
		target: RosterModel

		function onItemAdded(item) {
			const accountJid = item.accountJid
			const jid = item.jid

			if (accountJid === root.account.settings.jid && jid === root.jid) {
				MainController.openChatPageRequested(accountJid, jid)
			}
		}
	}
}
