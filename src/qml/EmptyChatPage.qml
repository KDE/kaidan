// SPDX-FileCopyrightText: 2018 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtGraphicalEffects 1.14
import org.kde.kirigami 2.19 as Kirigami

ChatPageBase {
	verticalScrollBarPolicy: Qt.ScrollBarAlwaysOff

	Item {
		height: root.height * 0.8

		// message bubble
		Kirigami.ShadowedRectangle {
			anchors.centerIn: parent
			width: label.width
			height: label.height
			shadow.color: Qt.darker(color, 1.2)
			shadow.size: 4
			radius: roundedCornersRadius
			color: primaryBackgroundColor

			Controls.Label {
				id: label
				text: qsTr("Please select a chat to start messaging")
				anchors.centerIn: parent
				padding: Kirigami.Units.gridUnit * 0.4
			}
		}
	}
}
