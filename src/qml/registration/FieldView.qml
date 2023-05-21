// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls

import "../elements/fields"

/**
 * This view is the base for views containing fields.
 */
View {
	/**
	 * Disallows the swiping and shows or hides the hint for invalid text input.
	 */
	function handleInvalidText() {
		swipeView.interactive = valid
		field.toggleHintForInvalidText()
	}

	function forceActiveFocus() {
		field.forceActiveFocus()
	}

	Controls.SwipeView.onIsCurrentItemChanged: {
		if (Controls.SwipeView.isCurrentItem) {
			if (!field.focus) {
				forceActiveFocus()
			}
		}
	}

	Component.onCompleted: {
		if (Controls.SwipeView.isCurrentItem) {
			forceActiveFocus()
		}
	}
}
