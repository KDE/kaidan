// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14

/**
 * This is used for a single action without an instantaneous result and the option to dismiss it.
 */
LoadingStackArea {
	default property alias __data: contentArea.data
	property alias cancelButton: cancelButton
	property alias confirmationButton: confirmationButton

	ColumnLayout {
		id: contentArea
	}

	RowLayout {
		Item {
			Layout.fillWidth: true
		}

		CenteredAdaptiveButton {
			id: cancelButton
			text: qsTr("Dismiss")
			Layout.preferredWidth: largeButtonWidth
			Layout.maximumWidth: Layout.preferredWidth
		}

		CenteredAdaptiveHighlightedButton {
			id: confirmationButton
			Layout.preferredWidth: cancelButton.Layout.preferredWidth
			Layout.maximumWidth: Layout.preferredWidth
		}

		Item {
			Layout.fillWidth: true
		}
	}
}
