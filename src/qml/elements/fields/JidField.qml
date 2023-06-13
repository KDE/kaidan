// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14

import im.kaidan.kaidan 1.0

import ".."

/**
 * This is a JID field with a hint for invalid JIDs.
 */
CredentialsField {
	id: root

	labelText: qsTr("Chat address")
	placeholderText: qsTr("user@example.org")
	inputMethodHints: Qt.ImhEmailCharactersOnly | Qt.ImhPreferLowercase | TextFieldCompleter.inputMethodHints
	invalidHintText: qsTr("The chat address must have the form <b>username@server</b>")
	text: AccountManager.jid
	completionRole: "display"
	completionModel: HostCompletionProxyModel {
		userInput: root.input
		sourceModel: HostCompletionModel
	}

	// Validate the entered JID and show a hint if it is not valid.
	onTextChanged: {
		valid = credentialsValidator.isUserJidValid(text)
		toggleHintForInvalidText()
	}
}
