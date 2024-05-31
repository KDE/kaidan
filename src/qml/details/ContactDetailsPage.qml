// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

DetailsPage {
	id: root

	required property string accountJid
	required property string jid

	ContactDetailsHeader {
		jid: root.jid
	}

	ContactDetailsContent {
		accountJid: root.accountJid
		jid: root.jid
		Layout.fillWidth: true
	}
}
