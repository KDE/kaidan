/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2023 Kaidan developers and contributors
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

	text: emoji + " " +  description
	width: smallButtonWidth + (description.length == 1 ? 0 : (description.length - 1) * Kirigami.Theme.defaultFont.pixelSize * 0.6)
	contentItem: Controls.Label {
		text: root.text
		padding: smallButtonWidth / 8
	}
	onClicked: {
		senderSheet.messageId = root.messageId
		senderSheet.isOwnMessage = root.isOwnMessage
		senderSheet.emoji = root.emoji
		senderSheet.senderJids = root.senderJids
		senderSheet.open()
	}
}
