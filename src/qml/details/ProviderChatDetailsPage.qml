// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Layouts

RosterItemDetailsPage {
	id: root
	title: qsTr("Provider Details")

	ProviderChatDetailsHeader {
		chatController: root.chatController
	}

	ProviderChatDetailsContent {
		chatController: root.chatController
		Layout.fillWidth: true
	}
}
