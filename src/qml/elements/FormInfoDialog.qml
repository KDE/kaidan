// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Dialog {
	default property alias __data: mainArea.data

	// Set negative insets to fix overlapping FormInfoContent background, scrollbar, footer and preserve the rounded corners of the dialog.
	leftInset: - __borderWidth
	rightInset: - __borderWidth
	bottomInset: - __borderWidth
	preferredWidth: Kirigami.Units.gridUnit * 33
	maximumWidth: preferredWidth
	topPadding: 0
	bottomPadding: 0
	leftPadding: 0
	rightPadding: 0
	footer: Kirigami.ShadowedRectangle {
		implicitHeight: Kirigami.Units.cornerRadius
		color: secondaryBackgroundColor
		corners {
			bottomLeftRadius: height / 2
			bottomRightRadius: height / 2
		}
	}

	ColumnLayout {
		id: mainArea
	}
}
