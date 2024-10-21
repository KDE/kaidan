// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

import im.kaidan.kaidan 1.0

DetailsDialog {
	id: root
	title: qsTr("Contact Details")

	ContactDetailsHeader {}

	ContactDetailsContent {
		dialog: root
		Layout.fillWidth: true
	}

	Connections {
		target: Kaidan

		// Close this dialog when the contact is removed.
		function onCloseChatPageRequested() {
			root.close()
		}
	}
}
