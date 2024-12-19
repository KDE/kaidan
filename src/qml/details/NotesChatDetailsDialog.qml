// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts

import im.kaidan.kaidan

import "../elements"

FormInfoDialog {
	id: root
	title: qsTr("Notes Details")

	NotesChatDetailsHeader {}

	NotesChatDetailsContent {
		dialog: root
		Layout.fillWidth: true
	}

	Connections {
		target: Kaidan

		// Close this dialog when the notes chat is removed.
		function onCloseChatPageRequested() {
			root.close()
		}
	}
}
