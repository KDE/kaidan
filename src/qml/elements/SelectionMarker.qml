// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

/**
 * Used to select an item inside of it.
 */
Controls.CheckBox {
	id: root
	padding: 3
	implicitWidth: 2 * padding + Math.max(implicitIndicatorWidth, implicitBackgroundWidth)
	implicitHeight: 2 * padding + Math.max(implicitIndicatorHeight, implicitBackgroundHeight)

	background: Rectangle {
		color: Kirigami.Theme.highlightColor
		opacity: root.hovered ? 1 : 0.8
		implicitWidth: 20
		implicitHeight: 20
		radius: implicitWidth
	}

	indicator: Kirigami.Icon {
		source: "emblem-ok-symbolic"
		color: Kirigami.Theme.highlightedTextColor
		implicitWidth: Kirigami.Units.iconSizes.small
		implicitHeight: implicitWidth
		anchors.centerIn: parent
	}
}
