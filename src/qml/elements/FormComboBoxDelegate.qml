// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

/**
 * This is a combo box with a workaround for an array-based model.
 */
MobileForm.FormComboBoxDelegate {
	// "FormComboBoxDelegate.indexOfValue()" seems to not work with an array-based model.
	// Thus, an own function is used.
	function indexOf(value) {
		if (Array.isArray(model)) {
			return model.findIndex((entry) => entry[valueRole] === value)
		}

		return indexOfValue(value)
	}
}
