// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami

import ".."

/**
 * This is a text field which can be focused and show a hint for invalid input.
 */
ColumnLayout {
	// text of the label for the input field
	property alias labelText: label.text

	// input field
	property alias inputField: inputField

	// placeholder text for the input field
	property alias placeholderText: inputField.placeholderText

	// input method hints for the input field
	property alias inputMethodHints: inputField.inputMethodHints

	// entered text
	property alias text: inputField.text

	// hint to be shown if the the entered is not valid
	property alias invalidHint: invalidHint

	// text to be shown as a hint if the entered text is not valid
	property alias invalidHintText: invalidHint.text

	// validity of the entered text
	property bool valid: true

	// requirement for showing the hint for invalid input
	property bool invalidHintMayBeShown: false

	// underlying data source for the completion view
	property alias completionModel: inputField.model

	// completion model role name to query
	property alias completionRole: inputField.role

	// completed text
	readonly property alias input: inputField.input

	// label for the input field
	Controls.Label {
		id: label
	}

	RowLayout {
		// input field
		TextFieldCompleter {
			id: inputField
			Layout.fillWidth: true
			selectByMouse: true

			// Show a hint for the first time if the entered text is not valid as soon as the input field loses the focus.
			onFocusChanged: {
				if (!focus && !invalidHintMayBeShown) {
					invalidHintMayBeShown = true
					toggleHintForInvalidText()
				}
			}
		}

		// icon for an invalid input
		Kirigami.Icon {
			id: invalidIcon
			visible: invalidHint.visible
			source: "dialog-error-symbolic"
			width: Kirigami.Units.iconSizes.smallMedium
			height: width
		}
	}

	// hint for entering a valid input
	Controls.Label {
		id: invalidHint
		visible: false
		Layout.fillWidth: true
		leftPadding: 5
		rightPadding: 5
		wrapMode: Text.Wrap
		color: Kirigami.Theme.neutralTextColor
	}

	/**
	 * Shows a hint if the entered text is not valid or hides it otherwise.
	 * If invalidHintMayBeShown was initially set to false, that is only done if the input field has lost the focus at least one time because of its onFocusChanged().
	 */
	function toggleHintForInvalidText() {
		invalidHint.visible = !valid && invalidHintMayBeShown && invalidHintText.length > 0;
	}

	/**
	 * Focuses the input field and selects its text.
	 * If the input field is already focused, the focusing is executed again to trigger its onFocusChanged().
	 */
	function forceActiveFocus() {
		if (inputField.focus)
			inputField.focus = false

		inputField.selectAll()
		inputField.forceActiveFocus()
	}
}
