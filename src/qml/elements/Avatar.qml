// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigamiaddons.components as Components

import im.kaidan.kaidan

Components.Avatar {
	id: root

	property string jid
	property bool isGroupChat: false

	source: jid ? avatarWatcher.url : ""
	iconSource: isGroupChat ? "system-users-symbolic" : "avatar-default-symbolic"
	color: Utils.userColor(jid, name)

	AvatarWatcher {
		id: avatarWatcher
		jid: root.jid ? root.jid : ""
	}
}
