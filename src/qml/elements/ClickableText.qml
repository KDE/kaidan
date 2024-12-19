// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

/**
 * This is a text that can be clicked.
 */
ScalableText  {
	property alias mouseArea: mouseArea
	signal clicked()

	ClickableMouseArea {
		id: mouseArea
	}
}
