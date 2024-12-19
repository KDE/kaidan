// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

UserListItem {
	id: root

	property alias userText: userText

	Controls.Label {
		id: userText
		text: root.name
		textFormat: Text.PlainText
		elide: Text.ElideMiddle
		maximumLineCount: 1
		Layout.fillWidth: true
		leftPadding: Kirigami.Units.smallSpacing * 1.5
	}
}
