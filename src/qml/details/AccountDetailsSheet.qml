// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14

import im.kaidan.kaidan 1.0

DetailsSheet {
	id: root
	jid: AccountManager.jid
	header: AccountDetailsHeader {
		sheet: root
		jid: root.jid
	}
	mainComponent: Component {
		AccountDetailsContent {
			sheet: root
			jid: root.jid
		}
	}
}
