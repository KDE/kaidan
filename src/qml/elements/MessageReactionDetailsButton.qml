// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

/**
 * This is a button for opening the details of all emojis in reaction to a message.
 */
MessageReactionButton {
	id: root

	property string messageId
	property bool isOwnMessage
	property var reactions
	property MessageReactionDetailsDialog detailsDialog

	text: qsTr("Show details")
	primaryColor: isOwnMessage ? primaryBackgroundColor : secondaryBackgroundColor
	contentItem: Kirigami.Icon {
		source: "view-more-symbolic"
	}
	onClicked: {
		detailsDialog.messageId = messageId
		detailsDialog.reactions = reactions
		detailsDialog.open()
	}
	onReactionsChanged: {
		if (detailsDialog.messageId === messageId) {
			detailsDialog.reactions = reactions
		}
	}
	Controls.ToolTip.text: text
}
