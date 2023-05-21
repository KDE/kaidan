// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls
import org.kde.kirigami 2.19 as Kirigami

/**
 * This is a centered and adaptive text.
 */
Controls.Label {
	// factor to scale the text
	property double scaleFactor: 1

	Layout.fillWidth: true
	horizontalAlignment: Text.AlignHCenter
	wrapMode: Text.WordWrap
	elide: Text.ElideRight
	font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * scaleFactor
}
