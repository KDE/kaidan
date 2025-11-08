// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/**
 * This is a secondary background with a dynamic color for a form delegate.
 */
FormCard.FormDelegateBackground {
	control: parent
	color: {
		const color = secondaryBackgroundColor
		let colorOpacity = 0.4

		if (!control.enabled) {
			colorOpacity = 0.2
		} else if (control.pressed) {
			colorOpacity = 1
		} else if (control.visualFocus) {
			colorOpacity = 0.4
		} else if (!Kirigami.Settings.tabletMode && control.hovered) {
			colorOpacity = 0.7
		}

		return Qt.rgba(color.r, color.g, color.b, colorOpacity)
	}
}
