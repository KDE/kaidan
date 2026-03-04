// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

/**
 * This is a button for opening an emoji picker to add emojis in reaction to a message.
 */
MessageReactionButton {
	id: root

	property int messageIndex
	property bool isOwnMessage
	property ListView messageListView
	property var openEmojiPicker

	text: qsTr("Add reaction…")
	primaryColor: isOwnMessage ? primaryBackgroundColor : secondaryBackgroundColor
	contentItem: Kirigami.Icon {
		source: "smiley-add"
	}
	Controls.ToolTip.text: text
	onClicked: {
		messageListView.setCurrentIndex(messageIndex)
		openEmojiPicker()
	}
}
