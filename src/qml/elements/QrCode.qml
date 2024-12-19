// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

RoundedRectangle {
	property string jid
	property alias source: icon.source

	border.color: "white"
	border.width: radius * 0.3

	Kirigami.Icon {
		id: icon
		anchors.fill: parent
		anchors.margins: parent.border.width
	}
}
