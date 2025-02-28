// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigamiaddons.formcard as FormCard

/**
 * This is a combo box with a workaround for an array-based model.
 */
FormCard.FormComboBoxDelegate {
	// "FormComboBoxDelegate.indexOfValue()" seems to not work with an array-based model.
	// Thus, an own function is used.
	function indexOf(value) {
		if (Array.isArray(model)) {
			return model.findIndex((entry) => entry[valueRole] === value)
		}

		return indexOfValue(value)
	}
}
