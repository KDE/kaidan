// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Layouts

RosterItemDetailsPage {
	id: root
	title: qsTr("Notes Details")

	NotesChatDetailsHeader {
		chatController: root.chatController
	}

	NotesChatDetailsContent {
		chatController: root.chatController
		Layout.fillWidth: true
	}
}
