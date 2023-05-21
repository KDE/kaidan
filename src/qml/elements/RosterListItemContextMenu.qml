// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls

import im.kaidan.kaidan 1.0

/**
 * This is a context menu with entries used for roster item.
 */
Controls.Menu {
	id: root

	property RosterListItem item: null

	Controls.MenuItem {
		text: root.item && root.item.pinned ? qsTr("Unpin") : qsTr("Pin")
		visible: root.item
		onTriggered: {
			if (root.item.pinned) {
				RosterModel.unpinItem(root.item.accountJid, root.item.jid)
			} else {
				RosterModel.pinItem(root.item.accountJid, root.item.jid)
			}
		}
	}
}
