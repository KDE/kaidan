// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

Kirigami.OverlaySheet {
	id: root

	default property alias __data: mainArea.data
	property string jid

	topPadding: 0
	bottomPadding: 0
	leftPadding: 0
	rightPadding: 0
	Kirigami.Theme.colorSet: Kirigami.Theme.Header
	onSheetOpenChanged: {
		if (!sheetOpen) {
			destroy()
		}
	}

	ColumnLayout {
		id: mainArea
		Layout.preferredWidth: 600
		Layout.preferredHeight: 600
		Layout.maximumWidth: 600
	}
}
