// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Layouts 1.15

import im.kaidan.kaidan 1.0

import "elements"

FormInfoDialog {
	title: qsTr("About Kaidan")

	AboutHeader {}
	AboutContent {
		Layout.fillWidth: true
	}
}
