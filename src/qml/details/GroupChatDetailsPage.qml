// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14

DetailsPage {
	id: root

	required property string accountJid

	GroupChatDetailsHeader {
		jid: root.jid
	}

	GroupChatDetailsContent {
		id: content
		accountJid: root.accountJid
		jid: root.jid
		Layout.fillWidth: true
	}

	function openContactListView() {
		content.openContactListView()
	}

	function openKeyAuthenticationUserListView() {
		content.openKeyAuthenticationUserListView()
	}
}
