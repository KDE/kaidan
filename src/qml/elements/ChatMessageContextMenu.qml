/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2022 Kaidan developers and contributors
 *  (see the LICENSE file for a full list of copyright authors)
 *
 *  Kaidan is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  In addition, as a special exception, the author of Kaidan gives
 *  permission to link the code of its release with the OpenSSL
 *  project's "OpenSSL" library (or with modified versions of it that
 *  use the same license as the "OpenSSL" library), and distribute the
 *  linked executables. You must obey the GNU General Public License in
 *  all respects for all of the code used other than "OpenSSL". If you
 *  modify this file, you may extend this exception to your version of
 *  the file, but you are not obligated to do so.  If you do not wish to
 *  do so, delete this exception statement from your version.
 *
 *  Kaidan is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kaidan.  If not, see <http://www.gnu.org/licenses/>.
 */

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
}
