// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

/**
 * This is a text that can be scaled.
 */
Controls.Label {
	// factor to scale the text
	property double scaleFactor: 1

	font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * scaleFactor
}
