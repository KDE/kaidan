// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

/**
 * Text-only button.
 */
Button {
	id: root
	flat: false
	font.weight: Font.DemiBold
	padding: Kirigami.Units.mediumSpacing * 2
	contentItem: Text {
		text: root.text
		color: Kirigami.Theme.backgroundColor
		font: root.font
		horizontalAlignment: Text.AlignHCenter
		verticalAlignment: Text.AlignVCenter
		elide: Text.ElideRight
	}
	Controls.ToolTip.visible: false
}
