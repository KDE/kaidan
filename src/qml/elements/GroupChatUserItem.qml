// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

UserListItem {
	id: root

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
