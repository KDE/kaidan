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

#include "Message.h"

#include <QStringBuilder>

#include "MediaUtils.h"

bool Message::operator==(const Message &m) const
{
	return m.id == id
		&& m.to == to
		&& m.from == from
		&& m.body == body
		&& m.stamp == stamp
		&& m.isSpoiler == isSpoiler
		&& m.spoilerHint == spoilerHint
		&& m.isMarkable == isMarkable
		&& m.marker == marker
		&& m.markerId == markerId
		&& m.outOfBandUrl == outOfBandUrl
		&& m.replaceId == replaceId
		&& m.originId == originId
		&& m.stanzaId == stanzaId
		&& m.receiptRequested == receiptRequested
		&& m.encryption == encryption
		&& m.senderKey == senderKey
		&& m.mediaType == mediaType
		&& m.isOwn == isOwn
		&& m.isEdited == isEdited
		&& m.deliveryState == deliveryState
		&& m.mediaLocation == mediaLocation
		&& m.mediaContentType == mediaContentType
		&& m.mediaLastModified == mediaLastModified
		&& m.mediaSize == mediaSize
		&& m.isSpoiler == isSpoiler
		&& m.spoilerHint == spoilerHint
		&& m.errorText == errorText;
}

bool Message::operator!=(const Message &m) const
{
	return !operator==(m);
}

QString Message::previewText() const
{
	if (isSpoiler) {
		if (spoilerHint.isEmpty()) {
			return tr("Spoiler");
		}
		return spoilerHint;
	}

	if (mediaType == Enums::MessageType::MessageText) {
		return body;
	}
	auto text = MediaUtils::mediaTypeName(mediaType);

	if (!body.isEmpty()) {
		return text % QStringLiteral(": ") % body;
	}
	return text;
}
