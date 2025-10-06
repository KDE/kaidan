// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Layouts

RosterItemDetailsDialog {
	id: root
	title: qsTr("Provider Details")

	ProviderChatDetailsHeader {
		dialog: root
		chatController: root.chatController
	}

	ProviderChatDetailsContent {
		dialog: root
		chatController: root.chatController
		Layout.fillWidth: true
	}
}
