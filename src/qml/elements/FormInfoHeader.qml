// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

GridLayout {
	property Kirigami.Dialog dialog

	flow: width > Kirigami.Units.gridUnit * 20 ? GridLayout.LeftToRight : GridLayout.TopToBottom
	Layout.topMargin: Kirigami.Settings.isMobile ? 0 : Kirigami.Units.smallSpacing * 5
	Layout.bottomMargin: Kirigami.Settings.isMobile ? 0 : Kirigami.Units.largeSpacing * 2
	Layout.leftMargin: Kirigami.Units.smallSpacing * 5
	Layout.rightMargin: Layout.leftMargin + (dialog && dialog.contentItem.Controls.ScrollBar.vertical.visible ? dialog.contentItem.Controls.ScrollBar.vertical.width + Kirigami.Units.largeSpacing * 2 : 0)
}
