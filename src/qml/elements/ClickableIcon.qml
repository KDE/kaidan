// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

/**
 * This is a clickable icon changing its color on several events if possible.
 */
Kirigami.Icon  {
	property alias mouseArea: mouseArea

	signal clicked()

	// Icons which cannot be colored (e.g., emojis) are highlighted in another way.
	active: mouseArea.containsMouse
	color: Kirigami.Theme.disabledTextColor
	implicitWidth: Kirigami.Units.iconSizes.smallMedium
	implicitHeight: Kirigami.Units.iconSizes.smallMedium
	Kirigami.Theme.colorSet: Kirigami.Theme.Window
	Kirigami.Theme.inherit: false

	ClickableMouseArea {
		id: mouseArea
	}
}
