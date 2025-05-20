// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Layouts

RosterItemDetailsPage {
	id: root
	title: qsTr("Group Details")

	GroupChatDetailsHeader {
		chatController: root.chatController
	}

	GroupChatDetailsContent {
		id: content
		chatController: root.chatController
		Layout.fillWidth: true
	}

	function openInviteeListView() {
		content.openInviteeListView()
	}

	function openKeyAuthenticationUserListView() {
		content.openKeyAuthenticationUserListView()
	}
}
