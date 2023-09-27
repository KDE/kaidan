// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import org.kde.kirigami 2.19 as Kirigami

Controls.Label {
	color: Kirigami.Theme.disabledTextColor
	leftPadding: font.pixelSize * 0.6
	rightPadding: leftPadding
	topPadding: leftPadding * 0.4
	bottomPadding: topPadding
	background: Rectangle {
		radius: parent.height * 0.5
		color: primaryBackgroundColor
	}
}