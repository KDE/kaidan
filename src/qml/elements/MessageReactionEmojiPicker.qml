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
