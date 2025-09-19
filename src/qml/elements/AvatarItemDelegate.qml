// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

ClickableItemDelegate {
	id: root

	property Account account
	property string jid
	property string name
	property bool isGroupChat
	property bool dragged: false
	property alias avatar: avatar

	interactiveBackground.color: {
		let colorOpacity = 0

		if (!root.enabled) {
			colorOpacity = 0
		} else if (root.dragged) {
			colorOpacity = 0.2
		} else if(root.highlighted) {
			colorOpacity = 0.5
		} else if (root.down || root.pressed) {
			colorOpacity = 0.2
		} else if (root.visualFocus) {
			colorOpacity = 0.1
		} else if (!Kirigami.Settings.tabletMode && root.hovered) {
			colorOpacity = 0.07
		} else if (root.checked) {
			colorOpacity = 0.05
		}

		const textColor = root.highlighted ? Kirigami.Theme.highlightColor : Kirigami.Theme.textColor
		return Qt.rgba(textColor.r, textColor.g, textColor.b, colorOpacity)
	}

	AccountRelatedAvatar {
		id: avatar
		jid: root.jid
		name: root.name
		isGroupChat: root.isGroupChat
		accountAvatar {
			jid: root.account ? root.account.settings.jid : ""
			name: root.account ? root.account.settings.displayName : ""
		}
		accountAvatarBorder.color: Qt.tint(primaryBackgroundColor, interactiveBackground.color)
	}
}
