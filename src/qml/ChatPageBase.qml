// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2023 Bhavy Airi <airiraghav@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtGraphicalEffects 1.14
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

/**
 * This is the base for a chat page.
 */
Kirigami.ScrollablePage {
	// color of the message bubbles on the right side
	readonly property color rightMessageBubbleColor: {
		Kirigami.Theme.colorSet = Kirigami.Theme.View
		const accentColor = Kirigami.Theme.highlightColor
		return Qt.tint(Kirigami.Theme.backgroundColor, Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.1))
	}

	// background of the chat page
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
