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
	property alias jid: jidField.text
	property alias name: nameField.text

	title: qsTr("Add new contact")
	parent: applicationWindow().overlay
	standardButtons: Kirigami.Dialog.NoButton
	padding: Kirigami.Units.mediumSpacing
	preferredWidth: largeButtonWidth
	onOpened: {
		// "jidField.forceActiveFocus()" would result in showing the invalid hint on second opening.
		jidField.inputField.forceActiveFocus()
	}
	onClosed: {
		jid = "";
		jidField.invalidHintMayBeShown = false
		jidField.toggleHintForInvalidText()
		name = "";
		messageField.text = "";
	}

	ColumnLayout {
		Layout.fillWidth: true

		JidField {
			id: jidField
			inputField.onAccepted: valid ? nameField.forceActiveFocus() : forceActiveFocus()
			Layout.fillWidth: true
			inputField.onActiveFocusChanged: {
				// The active focus is taken after opening by another item.
				// Thus, it must be forced again.
				if (!inputField.activeFocus && !nameField.inputField.activeFocus && !messageField.activeFocus) {
					jidField.forceActiveFocus()
					jidField.invalidHintMayBeShown = false
				} else {
					jidField.invalidHintMayBeShown = true
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

		Controls.Label {
			text: qsTr("Message (optional):")
			textFormat: Text.PlainText
			Layout.fillWidth: true
		}
		Controls.TextArea {
			id: messageField
			placeholderText: qsTr("Hello, I'm…")
			inputMethodHints: Qt.ImhPreferUppercase
			wrapMode: TextEdit.Wrap
			Layout.fillWidth: true
			Layout.minimumHeight: Kirigami.Units.gridUnit * 4
		}

		CenteredAdaptiveHighlightedButton {
			text: qsTr("Add")
			Layout.fillWidth: true
			onClicked: {
				if (jidField.valid) {
					Kaidan.client.rosterManager.addContactRequested(jid.toLowerCase(), name, messageField.text)
					close()

					Kaidan.openChatPageRequested(AccountManager.jid, jid)
				} else {
					jidField.forceActiveFocus()
				}
			}
		}

	}
}
