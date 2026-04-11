// SPDX-FileCopyrightText: 2026 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2026 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

Dialog {
	id: root

	property alias chatController: viewer.chatController
	property alias fileId: viewer.fileId

	signal highlightMessageRequested(messageId: string)

	header: null
	topPadding: 0
	bottomPadding: 0
	leftPadding: 0
	rightPadding: 0
	topInset: 0
	bottomInset: 0
	leftInset: 0
	rightInset: 0
	preferredWidth: applicationWindow().width - Kirigami.Units.gridUnit * 6
	preferredHeight: applicationWindow().height - Kirigami.Units.gridUnit * 6

	MediaViewer {
		id: viewer
		width: root.width
		height: root.height
		onHighlightMessageRequested: (messageId) => root.highlightMessageRequested(messageId)
	}
}
