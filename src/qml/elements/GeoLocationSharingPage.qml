// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtPositioning
import QtLocation
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

GeoLocationMapPage {
	id: root

	property MessageComposition messageComposition

	title: qsTr("Share location")
	map {
		onCenterChanged: {
			if (!locationFollowingButton.checked) {
				map.positionMarker.coordinate = map.center
			}
		}
		positionMarker.coordinate: map.center
		currentPositionSource.onPositionChanged: {
			if (position.coordinate.isValid && locationFollowingButton.checked) {
				map.positionMarker.coordinate = position.coordinate
				map.center = map.positionMarker.coordinate
			}
		}
		data: [
			IconButtonArea {
				id: buttonArea

				anchors {
					left: map.zoomSlider.right
					verticalCenter: map.zoomSlider.verticalCenter
					leftMargin: Kirigami.Units.largeSpacing
				}

				IconButton {
					id: locationFollowingButton
					text: qsTr("Follow current location")
					icon.source: checked ? MediaUtils.newMediaIconName(Enums.MessageType.MessageGeoLocation) : "find-location-symbolic"
					checkable: true
					checked: map.currentPositionSource.active
					enabled: map.currentPositionSource.supportedPositioningMethods !== PositionSource.NoPositioningMethods
					onCheckedChanged: {
						if (checked) {
							map.positionMarker.coordinate = map.currentPositionSource.position.coordinate
							map.center = map.positionMarker.coordinate
						} else {
							map.positionMarker.coordinate = map.center
						}
					}
				}

				IconButton {
					text: qsTr("Send location")
					icon.source: "mail-send-symbolic"
					onClicked: sendLocation()
				}
			},

			GeocodeModel {
				plugin: map.plugin
				autoUpdate: false
				onCountChanged: {
					if(count > 0) {
						const coordinate = get(0).coordinate
						map.center = coordinate
					}
				}
				Component.onCompleted: {
					query = Utils.systemCountryCode()
					update()
				}
			}
		]
	}
	Keys.onReturnPressed: sendLocation()

	function sendLocation() {
		root.messageComposition.sendGeoUri(map.center)
		popLayer()
	}
}
