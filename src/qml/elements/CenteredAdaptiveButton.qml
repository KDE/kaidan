// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

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
