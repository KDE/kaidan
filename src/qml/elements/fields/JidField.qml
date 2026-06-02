// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick

import im.kaidan.kaidan

import ".."

/**
 * This is a JID field with a hint for invalid JIDs.
 */
Field {
	id: root
	label: qsTr("Chat address")
	placeholderText: qsTr("user@example.org")
	invalidHintText: qsTr("Enter a valid chat address")
	inputMethodHints: Qt.ImhEmailCharactersOnly | Qt.ImhPreferLowercase | Field.inputMethodHints
	inputValidator.patterns: InputValidator.Pattern.Jid
	completionRole: "display"
	completionModel: HostCompletionProxyModel {
		userInput: root.input
		sourceModel: HostCompletionModel
	}
}
