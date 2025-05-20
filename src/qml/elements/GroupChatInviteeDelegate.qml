// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ContactDelegate {
	id: root

	ItemDelegateSelectionMarker {
		itemDelegate: root
		Layout.rightMargin: Kirigami.Units.largeSpacing * 2
	}
}
