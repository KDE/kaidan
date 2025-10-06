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
	property bool isProviderChat: false
	property bool isGroupChat: false

	source: avatarWatcher.url
	iconSource: {
		if (isProviderChat) {
			return "user-home-symbolic"
		}

		if (isGroupChat) {
			return "system-users-symbolic"
		}

		return "avatar-default-symbolic"
	}
	initialsMode: isProviderChat && !source.toString() ? Components.Avatar.InitialsMode.UseIcon : Components.Avatar.InitialsMode.UseInitials
	color: Utils.userColor(jid, name)

	AvatarWatcher {
		id: avatarWatcher
		jid: root.jid
	}
}
