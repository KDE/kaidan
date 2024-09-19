// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigami 2.19 as Kirigami

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
