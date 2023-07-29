// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

Kirigami.Dialog {
	id: root

	property alias jid: content.jid
	property alias name: content.name

	title: qsTr("Add contact")
	parent: applicationWindow().overlay
	standardButtons: Kirigami.Dialog.NoButton
	padding: Kirigami.Units.mediumSpacing
	preferredWidth: largeButtonWidth
	onOpened: content.jidField.forceActiveFocus()
	onClosed: destroy()

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
