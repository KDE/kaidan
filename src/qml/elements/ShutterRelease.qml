// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

Controls.ItemDelegate {
	id: root

	property var iconSource

	background: Kirigami.ShadowedRectangle {
		color: {
			if (parent.pressed) {
				return Qt.darker(secondaryBackgroundColor, 1.05)
			}

			return parent.hovered ? secondaryBackgroundColor : primaryBackgroundColor
		}
		opacity: 0.9
		radius: height / 2
		shadow.color: Qt.darker(color, 1.2)
		shadow.size: 4

		Behavior on color {
			ColorAnimation {
				duration: Kirigami.Units.shortDuration
			}
		}
	}
	contentItem: Loader {
		sourceComponent: parent.enabled ? icon : busyIndicator

		Component {
			id: icon

			Kirigami.Icon {
				source: root.iconSource
			}
		}

		Component {
			id: busyIndicator

			Controls.BusyIndicator {}
		}
	}
	padding: width * 0.1
	width: Kirigami.Units.iconSizes.large
	height: width
}
