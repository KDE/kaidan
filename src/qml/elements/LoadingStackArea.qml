// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/**
 * This is used for actions without instantaneous results.
 */
StackLayout {
	default property alias __data: contentArea.data
	property alias loadingArea: loadingArea
	property bool busy: false

	currentIndex: busy ? 1 : 0

	ColumnLayout {
		id: contentArea
		spacing: Kirigami.Units.largeSpacing * 2
	}

	LoadingArea {
		id: loadingArea
		background.color: secondaryBackgroundColor
	}
}
