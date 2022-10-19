// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

/**
 * This is a button for opening an emoji picker to add emojis in reaction to a message.
 */
MessageReactionButton {
	id: root

	property string messageId
	property MessageReactionEmojiPicker emojiPicker

	primaryColor: secondaryBackgroundColor
	contentItem: Kirigami.Icon {
		source: "smiley-add"
	}
	onClicked: {
		emojiPicker.messageId = root.messageId
		emojiPicker.open()
	}
}
