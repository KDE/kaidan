// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

/**
 * This is a button for opening the details of all emojis in reaction to a message.
 */
MessageReactionButton {
	id: root

	property Account account
	property string chatJid
	property bool isOwnMessage
	property var reactions

	text: qsTr("Show details")
	primaryColor: isOwnMessage ? primaryBackgroundColor : secondaryBackgroundColor
	contentItem: Kirigami.Icon {
		source: "view-more-symbolic"
	}
	onClicked: openOverlay(detailsDialog)
	Controls.ToolTip.text: text

	Component {
		id: detailsDialog

		MessageReactionDetailsDialog {
			account: root.account
			chatJid: root.chatJid
			reactions: root.reactions
		}
	}
}
