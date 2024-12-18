// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

import "../elements"

ContactDetailsHeader {
	id: root
	description {
		text: qsTr("Messages in this chat are synchronized as notes across all your devices")
		visible: ChatController.accountJid === root.jid
	}
}
