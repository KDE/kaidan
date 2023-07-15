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
	id: control
	padding: 3
	implicitWidth: 2 * padding + Math.max(implicitIndicatorWidth, implicitBackgroundWidth)
	implicitHeight: 2 * padding + Math.max(implicitIndicatorHeight, implicitBackgroundHeight)

	background: Rectangle {
		border.color: Kirigami.Theme.highlightColor
		implicitWidth: 20
		implicitHeight: 20
		radius: implicitWidth
	}

	indicator: Image {
		visible: control.checked
		source: Utils.getResourcePath("images/check-mark-pale.svg")
		sourceSize.width: 15
		sourceSize.height: 15
		anchors.centerIn: parent
	}
}
