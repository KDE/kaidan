// SPDX-FileCopyrightText: 2026 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick

import im.kaidan.kaidan

/**
 * This is a background with an image for various pages.
 */
Rectangle {
	color: secondaryBackgroundColor

	Image {
		source: Utils.getResourcePath("images/chat-page-background.svg")
		anchors.fill: parent
		fillMode: Image.Tile
		horizontalAlignment: Image.AlignLeft
		verticalAlignment: Image.AlignTop
	}
}
