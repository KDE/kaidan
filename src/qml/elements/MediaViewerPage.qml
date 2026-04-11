// SPDX-FileCopyrightText: 2026 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2026 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

Kirigami.Page {
	id: root

	property alias chatController: viewer.chatController
	property alias fileId: viewer.fileId

	signal highlightMessageRequested(messageId: string)

	topPadding: 0
	bottomPadding: 0
	leftPadding: 0
	rightPadding: 0
	Kirigami.Theme.colorSet: Kirigami.Theme.Window

	MediaViewer {
		id: viewer
		anchors.fill: parent
		onHighlightMessageRequested: (messageId) => root.highlightMessageRequested(messageId)
	}
}
