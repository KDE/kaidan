// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Layouts

RosterItemDetailsPage {
	id: root
	title: qsTr("Contact Details")

	ContactDetailsHeader  {
		chatController: root.chatController
	}

	ContactDetailsContent {
		chatController: root.chatController
		Layout.fillWidth: true
	}
}
