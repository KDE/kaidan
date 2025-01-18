// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

/**
 * This is a scrollable page with an image as its background.
 */
Kirigami.ScrollablePage {
	background: Rectangle {
		color: secondaryBackgroundColor

		Image {
			source: Utils.getResourcePath("images/chat-page-background.svg")
			anchors.fill: parent
			fillMode: Image.Tile
			horizontalAlignment: Image.AlignLeft
			verticalAlignment: Image.AlignTop
		}
	}
}
