// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

UserListItem {
	id: root

	// left: name
	Kirigami.Heading {
		text: root.name
		textFormat: Text.PlainText
		elide: Text.ElideRight
		maximumLineCount: 1
		level: 4
		Layout.fillWidth: true
	}

	// right: selection marker
	UserListItemSelectionMarker {
		userListItem: root
		Layout.rightMargin: Kirigami.Units.largeSpacing * 2
	}
}
