// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.12 as Kirigami

/**
 * This is used for a single action without an instantaneous result.
 */
StackLayout {
	id: root

	default property alias __data: contentArea.data
	property alias confirmationButton: confirmationButton
	property alias loadingDescription: loadingArea.description

	ColumnLayout {
		spacing: 20

		ColumnLayout {
			id: contentArea
			Layout.fillWidth: true
		}

		CenteredAdaptiveHighlightedButton {
			id: confirmationButton
			text: qsTr("OK")
		}
	}

	LoadingArea {
		id: loadingArea
		backgroundColor: secondaryBackgroundColor
	}

	function showLoadingView() {
		currentIndex = 1
	}

	function hideLoadingView() {
		currentIndex = 0
	}
}
