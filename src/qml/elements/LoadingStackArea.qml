// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/**
 * This is used for actions without instantaneous results.
 */
StackLayout {
	property alias loadingArea: loadingArea
	property bool busy: false

	currentIndex: busy ? 0 : 1

	LoadingArea {
		id: loadingArea
		background.color: secondaryBackgroundColor
	}
}
