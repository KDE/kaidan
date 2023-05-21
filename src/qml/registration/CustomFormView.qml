// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Controls 2.14 as Controls

import "../elements"

/**
 * This is the base for views used entering the values into the custom fields of the received registration form.
 */
View {
	descriptionText: qsTr("The server has requested more information.\nNot everything may be required.\nIf a CAPTCHA is shown, the displayed text must be entered.")
	imageSource: "custom-form"

	DataForm {
		id: registrationForm
		parent: contentArea
		model: formFilterModel
		baseModel: formModel
		displayTitle: false
		displayInstructions: false
	}

	function forceActiveFocus() {
		registrationForm.firstVisibleTextField.forceActiveFocus()
	}
}
