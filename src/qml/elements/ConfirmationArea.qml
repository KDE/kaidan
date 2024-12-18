// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/**
 * This is used for a single action without an instantaneous result.
 */
LoadingStackArea {
	default property alias __data: contentArea.data
	property alias confirmationButton: confirmationButton

	ColumnLayout {
		id: contentArea
		spacing: Kirigami.Units.largeSpacing
	}

	CenteredAdaptiveHighlightedButton {
		id: confirmationButton
	}
}
