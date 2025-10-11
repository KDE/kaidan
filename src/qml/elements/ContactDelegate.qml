// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ClickableItemDelegate {
	id: root

	property alias avatar: avatar
	property string jid
	property string name

	interactiveBackground.color: {
		const color = Kirigami.Theme.textColor
		let colorOpacity = 0

		if (!enabled) {
			colorOpacity = 0
		} else if (down || pressed) {
			colorOpacity = 0.2
		} else if (visualFocus) {
			colorOpacity = 0.1
		} else if (!Kirigami.Settings.tabletMode && hovered) {
			colorOpacity = 0.07
		} else if (checked) {
			colorOpacity = 0.05
		}

		return Qt.rgba(color.r, color.g, color.b, colorOpacity)
	}

	Avatar {
		id: avatar
		jid: root.jid
		name: root.name
	}

	Kirigami.Heading {
		text: root.name
		textFormat: Text.PlainText
		elide: Text.ElideRight
		maximumLineCount: 1
		level: 4
		Layout.fillWidth: true
	}
}
