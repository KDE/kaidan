// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

/**
 * This is a button for opening an emoji picker to add emojis in reaction to a message.
 */
MessageReactionButton {
	id: root

	property string messageId
	property bool isOwnMessage

	text: qsTr("Retry updating reactions")
	primaryColor: isOwnMessage ? primaryBackgroundColor : secondaryBackgroundColor
	// Unlike the other items inheriting MessageReactionButton, the icon would be too large without
	// the padding here.
	padding: Kirigami.Units.smallSpacing
	contentItem: Kirigami.Icon {
		source: "view-refresh-symbolic"
	}
	onClicked: MessageModel.resendMessageReactions(root.messageId)
	Controls.ToolTip.text: text
}
