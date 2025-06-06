// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

/**
 * This is a button used for a reaction or for reacting to a message.
 */
Button {
	id: root

	property color primaryColor
	property color accentColor

	height: Kirigami.Theme.defaultFont.pixelSize * 2.1
	width: smallButtonWidth
	hoverEnabled: true
	background: RoundedRectangle {
		color: {
			if (root.hovered) {
				return Qt.tint(root.primaryColor, Qt.rgba(root.accentColor.r, root.accentColor.g, root.accentColor.b, 0.1))
			}

			return Qt.tint(root.primaryColor, Qt.rgba(root.accentColor.r, root.accentColor.g, root.accentColor.b, 0.5))
		}

		Behavior on color {
			ColorAnimation { duration: Kirigami.Units.shortDuration }
		}
	}
}
