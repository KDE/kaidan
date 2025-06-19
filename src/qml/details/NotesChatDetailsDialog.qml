// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Layouts

RosterItemDetailsDialog {
	id: root
	title: qsTr("Notes Details")

	NotesChatDetailsHeader {
		dialog: root
		chatController: root.chatController
	}

	NotesChatDetailsContent {
		dialog: root
		chatController: root.chatController
		Layout.fillWidth: true
	}
}
