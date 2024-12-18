// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Layouts

import im.kaidan.kaidan

import "elements"

FormInfoPage {
	title: qsTr("About Kaidan")

	AboutHeader {}
	AboutContent {
		Layout.fillWidth: true
	}
}
