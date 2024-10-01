// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

import im.kaidan.kaidan 1.0

DetailsSheet {
	id: root

	NotesChatDetailsHeader {}

	NotesChatDetailsContent {
		sheet: root
		Layout.fillWidth: true
	}

	Connections {
		target: Kaidan

		// Close this sheet when the notes chat is removed.
		function onCloseChatPageRequested() {
			root.close()
		}
	}
}
