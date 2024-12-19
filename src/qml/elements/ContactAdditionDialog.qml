// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

Dialog {
	id: root

	property alias jid: content.jid
	property alias name: content.name

	title: qsTr("Add contact")
	padding: Kirigami.Units.mediumSpacing
	onOpened: content.jidField.forceActiveFocus()

	ContactAdditionContent {
		id: content
		jidField.inputField.onActiveFocusChanged: {
			// The active focus is taken by another item after opening.
			// Thus, it must be forced again.
			if (!jidField.inputField.activeFocus && !nameField.inputField.activeFocus && !messageField.activeFocus) {
				jidField.forceActiveFocus()
				jidField.invalidHintMayBeShown = false
			} else {
				jidField.invalidHintMayBeShown = true
			}
		}
	}

	Connections {
		target: Kaidan

		function onOpenChatPageRequested(accountJid, chatJid) {
			root.close()
		}
	}
}
