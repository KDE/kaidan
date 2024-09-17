// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

/**
 * This is a combo box with workarounds for an array-based model and usable within
 * Kirigami.OverlaySheet.
 */
MobileForm.FormComboBoxDelegate {
	property bool sheetUsed: false

	// "FormComboBoxDelegate.indexOfValue()" seems to not work with an array-based model.
	// Thus, an own function is used.
	function indexOf(value) {
		if (Array.isArray(model)) {
			return model.findIndex((entry) => entry[valueRole] === value)
		}

		return indexOfValue(value)
	}

	Component.onCompleted: {
		// "Kirigami.OverlaySheet" uses a z-index of 101.
		// In order to see the popup, it needs to have that z-index as well.
		if (sheetUsed) {
			let comboBox = contentItem.children[2];

			if (comboBox instanceof Controls.ComboBox) {
				comboBox.popup.z = 101
			}
		}
	}
}
