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
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

/**
 * This is a QR code generated for a specified JID or the own JID.
 *
 * If "isForLogin" is true, a QR code with a login XMPP URI for logging in on
 * another device is generated.

 * Otherwise, a QR code with a Trust Message URI is generated.
 * The Trust Message URI contains key IDs that other clients can use to make
 * trust decisions but they can also just add that contact.
 * If a JID is provided, that JID is used for the URI.
 * Otherwise, the own JID is used.
 */
Kirigami.Icon {
	source: {
		if (width > 0) {
			if (isForLogin) {
				return qrCodeGenerator.generateLoginUriQrCode(width)
			} else if (jid) {
				return qrCodeGenerator.generateContactTrustMessageQrCode(width, jid)
			} else {
				return qrCodeGenerator.generateOwnTrustMessageQrCode(width)
			}
		}

		return ""
	}

	property bool isForLogin: false
	property string jid

	QrCodeGenerator {
		id: qrCodeGenerator
	}

	Connections {
		target: MessageModel

		// Update the currently displayed QR code.
		function onKeysChanged() {
			widthChanged()
		}
	}
}
