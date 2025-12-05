// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Controls.Control {
	default property alias __data: mainArea.data

	topPadding: Kirigami.Settings.isMobile ? Kirigami.Units.largeSpacing : 0
	topInset: Kirigami.Settings.isMobile ? 0 : Kirigami.Units.largeSpacing * 4
	leftPadding: 0
	rightPadding: 0
	background: Rectangle {
		color: secondaryBackgroundColor
	}
	contentItem: ColumnLayout {
		id: mainArea
		spacing: Kirigami.Units.largeSpacing
	}
}
