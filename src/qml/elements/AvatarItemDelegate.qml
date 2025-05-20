// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
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
	property Account account
	property string jid
	property string name
	property bool isGroupChat
	property bool dragged: false
	property bool first: model === undefined || model.index === 0
	property alias avatar: avatar

	topPadding: 0
	leftPadding: 0
	bottomPadding: 0
	rightPadding: 0
	height: first ? Kirigami.Units.smallSpacing * 18 : Kirigami.Units.smallSpacing * 17
	background: null
	contentItem: Controls.Control {
		topInset: root.first ? Kirigami.Units.smallSpacing : 0
		bottomInset: Kirigami.Units.smallSpacing
		leftInset: Kirigami.Units.smallSpacing
		rightInset: Kirigami.Units.smallSpacing
		topPadding: root.first ? Kirigami.Units.smallSpacing : 0
		leftPadding: Kirigami.Units.smallSpacing * 3
		rightPadding: Kirigami.Units.smallSpacing * 3
		bottomPadding: Kirigami.Units.smallSpacing
		background: Rectangle {
			id: interactiveBackground
			radius: roundedCornersRadius
			color: {
				let colorOpacity = 0

				if (!root.enabled) {
					colorOpacity = 0
				} else if (root.dragged) {
					colorOpacity = 0.2
				} else if(root.highlighted) {
					colorOpacity = 0.5
				} else if (root.down || root.pressed) {
					colorOpacity = 0.2
				} else if (root.visualFocus) {
					colorOpacity = 0.1
				} else if (!Kirigami.Settings.tabletMode && root.hovered) {
					colorOpacity = 0.07
				} else if (root.checked) {
					colorOpacity = 0.05
				}

				const textColor = root.highlighted ? Kirigami.Theme.highlightColor : Kirigami.Theme.textColor
				return Qt.rgba(textColor.r, textColor.g, textColor.b, colorOpacity)
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

			AccountRelatedAvatar {
				id: avatar
				account: root.account
				jid: root.jid
				name: root.name
				isGroupChat: root.isGroupChat
				accountAvatarBorder.color: Qt.tint(primaryBackgroundColor, interactiveBackground.color)
			}
		}
	}
	Kirigami.Theme.inherit: false
}
