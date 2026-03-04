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
