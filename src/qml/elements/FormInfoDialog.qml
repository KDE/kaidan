// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Layouts 1.15
import org.kde.kirigami 2.19 as Kirigami

Dialog {
	default property alias __data: mainArea.data

	preferredWidth: Kirigami.Units.gridUnit * 32
	topPadding: 0
	bottomPadding: 0
	leftPadding: 0
	rightPadding: 0

	ColumnLayout {
		id: mainArea
	}
}
