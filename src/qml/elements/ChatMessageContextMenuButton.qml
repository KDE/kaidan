// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigami 2.19 as Kirigami

/**
 * This is a button used within a ChatMessageContextMenu.
 */
ClickableIcon {
	property Kirigami.Dialog contextMenu
	property bool expanded: false
	property bool shown: true
	property alias expansionTimer: expansionTimer

	visible: expanded && shown
	Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
	Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
	onClicked: contextMenu.close()

	Timer {
		id: expansionTimer
		onTriggered: parent.expanded = true
	}
}
