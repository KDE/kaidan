// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts

import "../elements"

FormInfoPage {
	id: root
	title: qsTr("Contact Details")

	ContactDetailsHeader {}

	ContactDetailsContent {
		Layout.fillWidth: true
	}
}
