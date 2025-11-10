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
	property string xmppUri

	confirmationButton.text: qsTr("Add")
	confirmationButton.onClicked: {
		if (jidField.valid) {
			busy = true

			// If the user inserted an XMPP URI into jidField, process that URI including possible trust decisions.
			// Otherwise, simply add the contact.
			switch (account.rosterController.addContactWithUri(xmppUri, name, messageField.text)) {
			case RosterController.ContactAdditionWithUriResult.AddingContact:
				// Try to authenticate or distrust keys.
				switch (account.atmController.makeTrustDecisionsWithUri(xmppUri)) {
				case AtmController.TrustDecisionWithUriResult.MakingTrustDecisions:
					showPassiveNotification(qsTr("Trust decisions made for contact"), Kirigami.Units.veryLongDuration * 4)
					break
				case AtmController.TrustDecisionWithUriResult.JidUnexpected:
					break
				case AtmController.TrustDecisionWithUriResult.InvalidUri:
					break
				}

				break
			case RosterController.ContactAdditionWithUriResult.ContactExists:
				break
			case RosterController.ContactAdditionWithUriResult.InvalidUri:
				account.rosterController.addContact(jid, name, messageField.text)
				break
			}
		} else {
			jidField.invalidHintMayBeShown = true
			jidField.forceActiveFocus()
		}
	}
	loadingArea.description: qsTr("Adding contact…")

	JidField {
		id: jidField
		Layout.fillWidth: true
		inputField.onTextEdited: {
			const jidOfXmppUri = Utils.jid(inputField.text)

			if (jidOfXmppUri) {
				// If the user inserts an XMPP URI into jidField, cache that URI in the background and set jidField.text to the URI's JID.
				root.xmppUri = inputField.text
				inputField.text = jidOfXmppUri
			} else {
				// Reset a cached URI if jidField.text is changed by the user afterwards.
				root.xmppUri = ""
			}
		}
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
