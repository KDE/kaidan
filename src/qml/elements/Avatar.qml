// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import org.kde.kirigamiaddons.components as Components

import im.kaidan.kaidan 1.0

Components.Avatar {
	property string jid
	property bool isGroupChat: false

	source: jid ? Kaidan.avatarStorage.getAvatarUrl(jid) : ""
	iconSource: isGroupChat ? "resource-group" : "user"
	color: Utils.userColor(jid, name)
}
