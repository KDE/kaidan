// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigami 2.19 as Kirigami

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
