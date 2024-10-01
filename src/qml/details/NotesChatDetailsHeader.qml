// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

import "../elements"

ContactDetailsHeader {
	id: root
	description {
		text: qsTr("Messages in this chat are synchronized as notes across all your devices")
		visible: ChatController.accountJid === root.jid
	}
}
