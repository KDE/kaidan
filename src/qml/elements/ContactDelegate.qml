// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

AvatarItemDelegate {
	id: root

	property alias textItem: textItem

	Kirigami.Heading {
		id: textItem
		text: root.name
		textFormat: Text.PlainText
		elide: Text.ElideRight
		maximumLineCount: 1
		level: 4
		Layout.fillWidth: true
	}
}
