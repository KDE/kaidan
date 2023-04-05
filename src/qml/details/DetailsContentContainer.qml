// SPDX-FileCopyrightText: 2023 Mathis Brüchert <mbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

import "../elements"

Controls.Control {
	id: root
	leftPadding: 0
	rightPadding: 0
	topPadding: Kirigami.Units.gridUnit
	bottomPadding: Kirigami.Units.gridUnit
	background: Rectangle {
		color: secondaryBackgroundColor
	}

	required property string jid
	required property Component mainComponent

	contentItem: Loader {
		sourceComponent: root.mainComponent
	}
}
