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
 * This is an emoji picker for adding emojis in reaction to a message.
 */
EmojiPicker {
	id: root

	property string messageId

	parent: root.parent
	anchors.centerIn: parent
	textArea: Controls.TextArea {
		visible: false
		onTextChanged: {
			// TODO: Refactor EmojiPicker to not append trailing whitespaces in this case
			if (text.length) {
				MessageModel.addMessageReaction(root.messageId, text.trim())
			}
			clear()
		}
	}
}
