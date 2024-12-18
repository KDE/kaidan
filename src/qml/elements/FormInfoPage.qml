// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
	default property alias __data: mainArea.data

	leftPadding: 0
	rightPadding: 0
	Kirigami.Theme.colorSet: Kirigami.Theme.Window

	ColumnLayout {
		id: mainArea
	}
}
