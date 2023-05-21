// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Robert Maerkisch <zatrox@kaidan.im>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import org.kde.kirigami 2.19 as Kirigami

Controls.Label {
	property int count: 0
	property bool muted: false

	text: count > 9999 ? "9999+" : count
	color: Kirigami.Theme.backgroundColor
	font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 0.9
	visible: count
	leftPadding: font.pixelSize * 0.45
	rightPadding: leftPadding
	topPadding: leftPadding * 0.3
	bottomPadding: topPadding

	background: Rectangle {
		radius: parent.height * 0.5
		color: parent.muted ? Kirigami.Theme.disabledTextColor : Kirigami.Theme.positiveTextColor
	}
}
