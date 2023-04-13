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

// SPDX-FileCopyrightText: 2021 Jan Blackquill <uhhadd@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtGraphicalEffects 1.14
import QtQuick.Controls 2.14 as Controls
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

Item {
	id: backgroundRoot

	property QtObject message
	property color color: message.isOwn ? rightMessageBubbleColor : primaryBackgroundColor
	property int tailSize: Kirigami.Units.largeSpacing
	property bool showTail: true
	property alias dummy: dummy
	readonly property alias metaInfoWidth: metaInfo.width

	clip: true

	Item {
		id: tailBase
		clip: true
		visible: false

		anchors {
			top: parent.top
			bottom: parent.bottom
			left: parent.left
			leftMargin: -backgroundRoot.tailSize * 2
			rightMargin: -backgroundRoot.tailSize
			right: mainBG.left
		}
		Rectangle {
			color: backgroundRoot.color

			anchors.fill: parent
			anchors.topMargin: 4
			anchors.rightMargin: -backgroundRoot.tailSize
		}
	}
	Item {
		id: tailMask
		clip: true
		visible: false

		anchors {
			top: parent.top
			bottom: parent.bottom
			left: parent.left
			leftMargin: -backgroundRoot.tailSize * 2
			rightMargin: -backgroundRoot.tailSize
			right: mainBG.left
		}
		Kirigami.ShadowedRectangle {
			anchors.fill: parent
			anchors.rightMargin: backgroundRoot.tailSize

			width: backgroundRoot.tailSize * 3
			color: "black"

			corners {
				topLeftRadius: 0
				topRightRadius: 0
				bottomRightRadius: backgroundRoot.tailSize * 10
				bottomLeftRadius: 0
			}
		}
	}
	OpacityMask {
		visible: showTail
		anchors.fill: tailBase
		source: tailBase
		maskSource: tailMask
		invert: true
	}
	Rectangle {
		id: mainBG
		radius: roundedCornersRadius
		color: backgroundRoot.color
		anchors.fill: parent
		anchors.leftMargin: backgroundRoot.tailSize
	}

	RowLayout {
		id: metaInfo
		anchors {
			bottom: parent.bottom
			right: mainBG.right
			margins: Kirigami.Units.smallSpacing
		}

		Controls.Label {
			id: timestamp
			opacity: 0.5
			text: Qt.formatDateTime(message.dateTime, "hh:mm")
			font.pointSize: -1
			font.pixelSize: Kirigami.Units.gridUnit * (2/3)

			MouseArea {
				id: timestampMouseArea
				anchors.fill: parent
			}

			Controls.ToolTip {
				visible: timestampMouseArea.containsMouse
				text: Qt.formatDateTime(message.dateTime, "dd. MMM yyyy, hh:mm")
				delay: 500
			}
		}

		Kirigami.Icon {
			source: backgroundRoot.message.encryption === Encryption.NoEncryption ? "channel-insecure-symbolic" : "channel-secure-symbolic"
			Layout.preferredWidth: Kirigami.Units.iconSizes.small
			Layout.preferredHeight: Layout.preferredWidth
		}

		Kirigami.Icon {
			// TODO: Use "security-low-symbolic" for distrusted, "security-medium-symbolic" for automatically trusted and "security-high-symbolic" for authenticated
			source: backgroundRoot.message.isTrusted ? "security-high-symbolic" : "security-low-symbolic"
			visible: backgroundRoot.message.encryption !== Encryption.NoEncryption
			Layout.preferredWidth: Kirigami.Units.iconSizes.small
			Layout.preferredHeight: Layout.preferredWidth
		}

		Image {
			visible: message.isOwn
			source: deliveryStateIcon
			Layout.preferredHeight: Kirigami.Units.gridUnit * 0.6
			Layout.preferredWidth: Kirigami.Units.gridUnit * 0.6
			sourceSize.height: Kirigami.Units.gridUnit * 0.6
			sourceSize.width: Kirigami.Units.gridUnit * 0.6

			MouseArea {
				id: checkmarkMouseArea
				anchors.fill: parent
				hoverEnabled: true
			}

			Controls.ToolTip {
				text: message.deliveryStateName
				visible: checkmarkMouseArea.containsMouse
				delay: 500
			}
		}
		Kirigami.Icon {
			source: "document-edit-symbolic"
			visible: message.edited
			Layout.preferredHeight: Kirigami.Units.gridUnit * 0.65
			Layout.preferredWidth: Kirigami.Units.gridUnit * 0.65
		}
	}

	Controls.Label {
		id: dummy
		text: "⠀"
	}
}
