// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

/**
 * This is an avatar containing a smaller avatar for its related account.
 *
 * The avatar for the account is only shown if multiple accounts are in use.
 */
Item {
	property alias chatAvatar: chatAvatar
	property alias accountAvatar: accountAvatar
	property alias accountAvatarBorder: accountAvatarBorder

	implicitWidth: chatAvatar.implicitWidth + accountAvatar.implicitWidth - accountAvatar.anchors.horizontalCenterOffset + (accountAvatarBorder.width - accountAvatar.implicitWidth)
	implicitHeight: chatAvatar.implicitHeight + accountAvatar.implicitHeight - accountAvatar.anchors.verticalCenterOffset + (accountAvatarBorder.height - accountAvatar.implicitHeight)

	Avatar {
		id: chatAvatar
		anchors.centerIn: parent

		Avatar {
			id: accountAvatar
			implicitWidth: parent.implicitWidth * 0.4
			implicitHeight: parent.implicitHeight * 0.4
			visible: AccountController.accounts.length > 1 && jid !== parent.jid
			anchors.horizontalCenter: parent.horizontalCenter
			anchors.horizontalCenterOffset: parent.implicitWidth / 2 - implicitWidth * 0.4
			anchors.verticalCenter: parent.verticalCenter
			anchors.verticalCenterOffset: parent.implicitHeight / 2 - implicitHeight * 0.4

			Rectangle {
				id: accountAvatarBorder
				z: parent.z - 1
				color: Kirigami.Theme.backgroundColor
				radius: height / 2
				width: parent.implicitWidth * 1.2
				height: parent.implicitHeight * 1.2
				anchors.centerIn: parent
			}
		}
	}
}
