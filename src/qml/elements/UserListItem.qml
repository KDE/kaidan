// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

Kirigami.AbstractListItem {
	id: root

	default property alias __data: content.data
	property alias avatar: avatar

	property string accountJid
	property string jid
	property string name
	property bool selected: false

	topPadding: 0
	leftPadding: 0
	bottomPadding: 0
	height: 65
	background: Rectangle {
		color: {
			let colorOpacity = 0

			if (!root.enabled) {
				colorOpacity = 0
			} else if (root.pressed) {
				colorOpacity = 0.2
			} else if (root.visualFocus) {
				colorOpacity = 0.1
			} else if (!Kirigami.Settings.tabletMode && root.hovered) {
				colorOpacity = 0.07
			} else if (root.selected) {
				colorOpacity = 0.05
			}

			const textColor = Kirigami.Theme.textColor
			return Qt.rgba(textColor.r, textColor.g, textColor.b, colorOpacity)
		}

		Behavior on color {
			ColorAnimation { duration: Kirigami.Units.shortDuration }
		}
	}

	RowLayout {
		id: content
		spacing: Kirigami.Units.gridUnit * 0.5

		// left border: presence
		Rectangle {
			id: presenceIndicator
			width: Kirigami.Units.gridUnit * 0.3
			height: parent.height
			color: userPresence.availabilityColor

			UserPresenceWatcher {
				id: userPresence
				jid: root.jid
			}
		}

		// left: avatar
		Item {
			Layout.preferredWidth: parent.height - Kirigami.Units.gridUnit * 0.8
			Layout.preferredHeight: Layout.preferredWidth

			Avatar {
				id: avatar
				anchors.fill: parent
				jid: root.jid
				name: root.name
				width: height
			}
		}
	}
}
