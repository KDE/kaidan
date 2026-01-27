// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami

/**
 * This is a search field for filtering a list view.
 */
Kirigami.SearchField {
	id: root

	property ListView listView

	hoverEnabled: true
	implicitHeight: Kirigami.Units.gridUnit * 2
	background: Rectangle {
		color: secondaryBackgroundColor
		radius: height / 2
		border {
			color: {
				const color = Kirigami.Theme.textColor
				let colorOpacity = reducedBackgroundOpacity * 0.15

				if (root.hovered) {
					colorOpacity = reducedBackgroundOpacity * 0.4
				} else if (root.activeFocus) {
					colorOpacity = reducedBackgroundOpacity * 0.25
				}

				return Qt.rgba(color.r, color.g, color.b, colorOpacity)
			}
			width: 2

			Behavior on color {
				ColorAnimation {}
			}
		}
	}
	Keys.onEscapePressed: clear()
	Keys.onPressed: event => {
		switch (event.key) {
		case Qt.Key_Return:
		case Qt.Key_Enter:
			if (listView.currentItem) {
				listView.currentItem.clicked()
			} else if (listView.count) {
				// Simulate clicking on the first item of the listView.
				listView.itemAtIndex(0).clicked()
			}
		}
	}
}
