// SPDX-FileCopyrightText: 2026 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

RosterItemDetailsHeader {
	id: root

	property alias description: description.text

	account: chatController.account
	jid: chatController.jid
	displayName: chatController.rosterItem.displayName

	Controls.Label {
		id: description
		color: Kirigami.Theme.neutralTextColor
		wrapMode: Text.Wrap
		Layout.fillWidth: true
	}
}
