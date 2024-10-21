// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

Kirigami.Dialog {
	id: root

	default property alias __data: mainArea.data

	standardButtons: Kirigami.Dialog.NoButton
	preferredWidth: Kirigami.Units.gridUnit * 32
	maximumHeight: applicationWindow().height - Kirigami.Units.gridUnit * 5
	topPadding: 0
	bottomPadding: 0
	leftPadding: 0
	rightPadding: 0
	onClosed: destroy()

	ColumnLayout {
		id: mainArea
	}
}
