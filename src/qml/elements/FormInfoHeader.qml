// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Layouts
import org.kde.kirigami as Kirigami

GridLayout {
	flow: width > Kirigami.Units.gridUnit * 20 ? GridLayout.LeftToRight : GridLayout.TopToBottom
	Layout.topMargin: Kirigami.Settings.isMobile ? 0 : Kirigami.Units.largeSpacing * 3
	Layout.bottomMargin: Kirigami.Settings.isMobile ? 0 : Kirigami.Units.largeSpacing * 2
	Layout.leftMargin: Kirigami.Units.largeSpacing * 3
	Layout.rightMargin: Layout.leftMargin
}
