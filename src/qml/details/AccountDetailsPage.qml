// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts

import im.kaidan.kaidan

import "../elements"

FormInfoPage {
	id: root

	property string jid

	title: qsTr("Account Details")

	AccountDetailsHeader {
		jid: root.jid
	}

	AccountDetailsContent {
		jid: root.jid
		Layout.fillWidth: true
	}
}
