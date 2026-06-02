// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/**
 * This is used for a single action without an instantaneous result.
 */
LoadingStackArea {
	default property alias __data: contentArea.data
	property alias confirmationButton: confirmationButton

	ColumnLayout {
		spacing: Kirigami.Units.largeSpacing * 2

		ColumnLayout {
			spacing: 0
			id: contentArea
		}

		CenteredAdaptiveButton {
			id: confirmationButton
			Layout.topMargin: FormCard.FormCardUnits.verticalPadding
			Layout.bottomMargin: FormCard.FormCardUnits.verticalPadding
			Layout.leftMargin: FormCard.FormCardUnits.horizontalPadding
			Layout.rightMargin: FormCard.FormCardUnits.horizontalPadding
		}
	}
}
