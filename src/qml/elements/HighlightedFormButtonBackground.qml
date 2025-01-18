// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/**
 * This is a highlighted background with a dynamic color for a form delegate.
 */
FormCard.FormDelegateBackground {
	control: parent
	color: {
		let colorOpacity = 0.1;

		if (!control.enabled) {
			colorOpacity = 0;
		} else if (control.pressed) {
			colorOpacity = 0.2;
		} else if (control.visualFocus) {
			colorOpacity = 0.1;
		} else if (!Kirigami.Settings.tabletMode && control.hovered) {
			colorOpacity = 0.15;
		}

		const highlightColor = Kirigami.Theme.highlightColor
		return Qt.rgba(highlightColor.r, highlightColor.g, highlightColor.b, colorOpacity)
	}
}
