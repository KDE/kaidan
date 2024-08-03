// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

import im.kaidan.kaidan 1.0

DetailsPage {
	id: root

	AccountDetailsHeader {
		jid: roo.jid
	}

	AccountDetailsContent {
		jid: root.jid
		Layout.fillWidth: true
	}
}
