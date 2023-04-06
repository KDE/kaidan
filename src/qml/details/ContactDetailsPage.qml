// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Layouts 1.14

DetailsPage {
	id: root

	required property string jid

	ContactDetailsHeader {
		jid: root.jid
	}

	ContactDetailsContent {
		jid: root.jid
		Layout.fillWidth: true
	}
}
