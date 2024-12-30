// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Effects

GeoLocationMap {
	id: root

	property Item message

	layer.enabled: true
	layer.effect: MultiEffect {
		maskSource: ShaderEffectSource {
			sourceItem: Rectangle {
				radius: roundedCornersRadius
				width: root.width
				height: root.width
				anchors.centerIn: parent
			}
		}
	}

	OpacityChangingMouseArea {
		// Keep the copyright link accessible.
		z: parent.z - 1
		opacityItem: parent
		acceptedButtons: Qt.LeftButton | Qt.RightButton
		onClicked: (event) => {
			if (event.button === Qt.LeftButton) {
				message.openGeoLocationMap()
			} else if (event.button === Qt.RightButton) {
				root.message.showContextMenu(this)
			}
		}
	}
}
