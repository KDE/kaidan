// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

Controls.ItemDelegate {
	id: root

	default property alias __data: content.data
	property alias interactiveBackground: interactiveBackground
	property bool first: model === undefined || model.index === 0

	topPadding: 0
	leftPadding: 0
	bottomPadding: 0
	rightPadding: 0
	background: null
	contentItem: Controls.Control {
		topInset: root.first ? Kirigami.Units.smallSpacing : 0
		bottomInset: Kirigami.Units.smallSpacing
		leftInset: Kirigami.Units.smallSpacing
		rightInset: Kirigami.Units.smallSpacing
		topPadding: root.first ? Kirigami.Units.smallSpacing * 3 : Kirigami.Units.smallSpacing * 2
		leftPadding: Kirigami.Units.smallSpacing * 3
		rightPadding: Kirigami.Units.smallSpacing * 3
		bottomPadding: Kirigami.Units.smallSpacing * 3
		background: Rectangle {
			id: interactiveBackground
			radius: roundedCornersRadius
			color: {
				let colorOpacity = 0

				if (parent.pressed) {
					colorOpacity = 0.2
				} else if (parent.visualFocus) {
					colorOpacity = 0.1
				} else if (!Kirigami.Settings.tabletMode && parent.hovered) {
					colorOpacity = 0.07
				}

				return Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, colorOpacity)
			}

			Behavior on color {
				ColorAnimation {
					duration: Kirigami.Units.shortDuration
				}
			}
		}
		contentItem: RowLayout {
			id: content
			spacing: Kirigami.Units.largeSpacing
		}
	}
	Kirigami.Theme.inherit: false
}
