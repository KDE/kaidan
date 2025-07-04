// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami

GeoLocationMap {
	id: root

	property Item message

	layer.enabled: GraphicsInfo.api !== GraphicsInfo.Software
	layer.effect: Kirigami.ShadowedTexture {
		radius: roundedCornersRadius
	}

	Behavior on opacity {
		NumberAnimation {}
	}

	OpacityChangingMouseArea {
		// Keep the copyright link accessible.
		z: parent.z - 1
		opacityItem: parent
		acceptedButtons: Qt.LeftButton | Qt.RightButton
		onClicked: (event) => {
			if (event.button === Qt.LeftButton) {
				root.message.openGeoLocationMap()
			} else if (event.button === Qt.RightButton) {
				root.message.showContextMenu(this)
			}
		}
	}
}
