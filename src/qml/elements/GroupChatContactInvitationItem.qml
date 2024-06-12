// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls
import org.kde.kirigami 2.19 as Kirigami

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
