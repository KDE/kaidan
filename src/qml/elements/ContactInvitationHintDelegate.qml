// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as Components

ClickableItemDelegate {
	id: root
	hoverEnabled: true
	checked: true

	Components.Avatar {
		iconSource: "resource-group-new"
		initialsMode: Components.Avatar.InitialsMode.UseIcon
		color: Kirigami.Theme.textColor
	}

	Kirigami.Heading {
		text: qsTr("Invite contacts to this group…")
		font.italic: true
		elide: Text.ElideRight
		maximumLineCount: 1
		level: 4
		Layout.fillWidth: true
	}
}
