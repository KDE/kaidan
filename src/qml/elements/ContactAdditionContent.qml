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

ConfirmationArea {
	property alias jidField: jidField
	property alias jid: jidField.text
	property alias nameField: nameField
	property alias name: nameField.text
	property alias messageField: messageField

	confirmationButton.text: qsTr("Add")
	confirmationButton.onClicked: {
		const jidInLowerCase = jid.toLowerCase()

		if (RosterModel.hasItem(jidInLowerCase)) {
			showPassiveNotification(qsTr("Contact already exists"),
									"long",
									qsTr("Open chat"),
									function () {
										Kaidan.openChatPageRequested(AccountManager.jid, jidInLowerCase)
									})
		} else if (jidField.valid) {
			showLoadingView()
			Kaidan.client.rosterManager.addContactRequested(jidInLowerCase, name, messageField.text)
			hideLoadingView()
			Kaidan.openChatPageRequested(AccountManager.jid, jidInLowerCase)
		} else {
			jidField.forceActiveFocus()
		}
	}
	loadingArea.description: qsTr("Adding contact…")

	JidField {
		id: jidField
		text: ""
		inputField.onAccepted: valid ? nameField.forceActiveFocus() : forceActiveFocus()
		Layout.fillWidth: true
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
	}
}
