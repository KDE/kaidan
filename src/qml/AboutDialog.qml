// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Layouts

import im.kaidan.kaidan

import "elements"

FormInfoDialog {
	id: root
	title: qsTr("About Kaidan")

	AboutHeader {
		dialog: root
	}

	AboutContent {
		Layout.fillWidth: true
	}
}
