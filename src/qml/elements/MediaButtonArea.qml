// SPDX-FileCopyrightText: 2026 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

Controls.Control {
	property alias mainBackground: mainBackground

	background: Rectangle {
		id: mainBackground
		color: primaryBackgroundColor
		opacity: 0.9
		radius: parent.height / 2
	}
	clip: true
	topPadding: Kirigami.Units.smallSpacing
	bottomPadding: Kirigami.Units.smallSpacing
	leftPadding: Kirigami.Units.smallSpacing
	rightPadding: Kirigami.Units.smallSpacing

	Behavior on opacity {
		NumberAnimation {}
	}
}
