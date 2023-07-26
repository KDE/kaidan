// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami

Item {
	id: root

	property alias background: background
	property alias description: description.text

	Rectangle {
		id: background
		anchors.fill: content
		anchors.margins: -8
		radius: roundedCornersRadius
		color: primaryBackgroundColor
		opacity: 0.9
	}

	ColumnLayout {
		id: content
		anchors.centerIn: parent

		Controls.BusyIndicator {
			Layout.alignment: Qt.AlignHCenter
		}

		Controls.Label {
			id: description
			text: qsTr("Loading…")
			font.italic: true
			color: Kirigami.Theme.textColor
		}
	}
}
