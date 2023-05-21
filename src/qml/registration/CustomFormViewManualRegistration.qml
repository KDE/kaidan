// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Controls 2.14 as Controls

/**
 * This view is used for entering the values into the custom fields of the received registration form during the manual registration.
 */
CustomFormView {
	// This is used for automatically focusing the first field of the form.
	Controls.SwipeView.onIsCurrentItemChanged: {
		if (Controls.SwipeView.isCurrentItem) {
			forceActiveFocus()
		}
	}
}
