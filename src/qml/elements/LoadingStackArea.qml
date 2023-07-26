// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami

/**
 * This is used for actions without instantaneous results.
 */
StackLayout {
	default property alias __data: contentArea.data
	property alias loadingArea: loadingArea

	ColumnLayout {
		id: contentArea
		spacing: Kirigami.Units.largeSpacing * 2
	}

	LoadingArea {
		id: loadingArea
		background.color: secondaryBackgroundColor
	}

	function showLoadingView() {
		currentIndex = 1
	}

	function hideLoadingView() {
		currentIndex = 0
	}
}
