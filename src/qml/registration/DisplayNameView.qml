// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import im.kaidan.kaidan 1.0

import "../elements/fields"

/**
 * This view is used for entering the display name.
 */
FieldView {
	descriptionText: qsTr("Your display name is what your contacts will see.\nIn case you don't want to use a display name, you can leave this field empty.\nUsing a display name may let new contacts recognize you easier but also could decrease your privacy!")
	imageSource: "display-name"

	property alias text: field.text

	Field {
		id: field
		parent: contentArea
		labelText: qsTr("Display Name")
		inputMethodHints: Qt.ImhPreferUppercase
	}
}
