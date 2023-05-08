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
 * This is a button displaying an emoji and its count in reaction to a message for opening a
 * corresponding sheet.
 */
MessageReaction {
	id: root

	property bool isOwnMessage
	property string emoji
	property var senderJids: []
	property string description: senderJids.length
	property MessageReactionSenderSheet senderSheet

	text: description === "1" ? emoji : emoji + " " + description
	width: smallButtonWidth + (description.length == 1 ? 0 : (description.length - 1) * Kirigami.Theme.defaultFont.pixelSize * 0.6)
	contentItem: Controls.Label {
		text: root.text
		horizontalAlignment: Text.AlignHCenter
	}
	onClicked: {
		senderSheet.messageId = root.messageId
		senderSheet.isOwnMessage = root.isOwnMessage
		senderSheet.emoji = root.emoji
		senderSheet.senderJids = root.senderJids
		senderSheet.open()
	}
}
