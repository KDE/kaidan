// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.19 as Kirigami

Item {
	id: root

	property alias background: background
	property alias description: description.text

	RoundedRectangle {
		id: background
		anchors.fill: content
		anchors.margins: -8
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
