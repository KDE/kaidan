// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts

import im.kaidan.kaidan

import "../elements"

FormInfoPage {
	id: root
	title: qsTr("Notes Details")

	NotesChatDetailsHeader {}

	NotesChatDetailsContent {
		Layout.fillWidth: true
	}
}
