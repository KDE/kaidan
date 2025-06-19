// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Layouts

RosterItemDetailsDialog {
	id: root
	title: qsTr("Contact Details")

	ContactDetailsHeader {
		chatController: root.chatController
	}

	ContactDetailsContent {
		dialog: root
		chatController: root.chatController
		Layout.fillWidth: true
	}
}
