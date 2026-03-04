// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import QtMultimedia
import org.kde.kirigami as Kirigami

/**
 * This is a hint for the status of a camera.
 */
Rectangle {
	color: primaryBackgroundColor

	ColumnLayout {
		anchors.fill: parent
		anchors.margins: Kirigami.Units.largeSpacing

		Status {
			icon {
				source: "camera-disabled-symbolic"
				fallback: "camera-off"
			}
			text.text: qsTr("No camera available")
		}
	}
}
