// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

Kirigami.Heading {
	level: 5
	leftPadding: font.pixelSize * 0.7
	rightPadding: leftPadding
	topPadding: leftPadding * 0.4
	bottomPadding: topPadding
	background: Kirigami.ShadowedRectangle  {
		color: primaryBackgroundColor
		radius: parent.height * 0.5
		shadow.color: Qt.darker(color, 1.2)
		shadow.size: 4
	}
}
