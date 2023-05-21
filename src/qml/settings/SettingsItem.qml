// SPDX-FileCopyrightText: 2018 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami

Kirigami.BasicListItem {
	property string name
	property string description

	reserveSpaceForIcon: icon

	contentItem: ColumnLayout {
		Kirigami.Heading {
			text: name
			textFormat: Text.PlainText
			elide: Text.ElideRight
			maximumLineCount: 1
			level: 2
			Layout.fillWidth: true
			Layout.maximumHeight: Kirigami.Units.gridUnit * 1.5
		}

		Controls.Label {
			Layout.fillWidth: true
			text: description
			wrapMode: Text.WordWrap
			textFormat: Text.PlainText
		}
	}
}
