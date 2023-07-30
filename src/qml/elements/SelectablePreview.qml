// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import org.kde.kirigami 2.19 as Kirigami

/**
 * Used to show a selectable preview (e.g., of an image or video).
 */
Controls.ItemDelegate {
	id: root

	default property alias __data: selectionArea.data
	property alias containsMouse: selectionArea.containsMouse

	implicitWidth: GridView.view.cellWidth
	implicitHeight: GridView.view.cellHeight
	autoExclusive: false
	padding: 0
	background: Rectangle {}
	contentItem: MouseArea {
		id: selectionArea
		hoverEnabled: true
		acceptedButtons: Qt.NoButton

		// overlay to indicate the currently hovered or selected medium
		Rectangle {
			color: Kirigami.Theme.highlightColor
			opacity: 0.1
			z: 1
			visible: root.hovered || root.checked
			anchors.fill: parent
		}
	}
}
