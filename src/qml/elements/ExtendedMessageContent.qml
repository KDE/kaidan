// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigami 2.19 as Kirigami

RowLayout {
	id: root

	property alias mainArea: mainArea
	property alias mainAreaBackground: mainAreaBackground
	property alias contentArea: contentArea
	property real minimumWidth
	property real maximumWidth

	Controls.Control {
		id: mainArea
		padding: Kirigami.Units.smallSpacing
		background: Rectangle {
			id: mainAreaBackground
			color: secondaryBackgroundColor
			radius: roundedCornersRadius
		}
		contentItem: RowLayout {
			id: contentArea
			spacing: Kirigami.Units.smallSpacing * 3
		}
	}
}
