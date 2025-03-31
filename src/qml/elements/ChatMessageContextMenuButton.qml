// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

/**
 * This is a button used within a ChatMessageContextMenu.
 */
ClickableIcon {
	property Kirigami.Dialog contextMenu
	property bool expanded: false
	property bool shown: true
	property alias expansionTimer: expansionTimer

	visible: expanded && shown
	onClicked: contextMenu.close()

	Timer {
		id: expansionTimer
		onTriggered: parent.expanded = true
	}
}
