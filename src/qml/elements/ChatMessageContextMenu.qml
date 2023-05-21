// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Yury Gubich <blue@macaw.me>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls

import im.kaidan.kaidan 1.0

/**
 * This is a context menu with entries used for chat messages.
 */
Controls.Menu {
	id: root

	property ChatMessage message: null
	property var file: null

	Controls.MenuItem {
		text: qsTr("Copy message")
		visible: root.message && root.message.bodyLabel.visible
		onTriggered: {
			if (root.message && !root.message.isSpoiler || message && root.message.isShowingSpoiler)
				Utils.copyToClipboard(root.message && root.message.messageBody)
			else
				Utils.copyToClipboard(root.message && root.message.spoilerHint)
		}
	}

	Controls.MenuItem {
		text: qsTr("Edit message")
		enabled: MessageModel.canCorrectMessage(root.message && root.message.modelIndex)
		onTriggered: root.message.messageEditRequested(root.message.msgId, root.message.messageBody)
	}

	Controls.MenuItem {
		text: qsTr("Copy download URL")
		visible: root.file && root.file.downloadUrl
		onTriggered: Utils.copyToClipboard(root.file.downloadUrl)
	}

	Controls.MenuItem {
		text: qsTr("Quote message")
		onTriggered: {
			root.message.quoteRequested(root.message.messageBody)
		}
	}

	Controls.MenuItem {
		text: qsTr("Delete file")
		visible: root.file && root.file.localFilePath
		onTriggered: {
			Kaidan.fileSharingController.deleteFile(root.message.msgId, root.file)
		}
	}

	Controls.MenuItem {
		text: qsTr("Mark as first unread")
		visible: root.message && !root.message.isOwn
		onTriggered: {
			MessageModel.markMessageAsFirstUnread(message.modelIndex);
			MessageModel.resetCurrentChat()
			openChatView()
		}
	}
}
