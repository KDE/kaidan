// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

/**
 * This is a centered button having an adjustable label and fitting its parent's width.
 */
Button {
	Layout.alignment: Qt.AlignHCenter
	Layout.fillWidth: true

	Kirigami.Theme.textColor: {
		if (Style.isMaterial)
			return Kirigami.Theme.positiveTextColor
	}
}
