// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

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
