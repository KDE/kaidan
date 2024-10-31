// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import QtPositioning 5.15
import QtLocation 5.15
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

Dialog {
	id: root

	property MessageComposition messageComposition
	property var selectedGeoLocation

	title: qsTr("Share Location")
	preferredHeight: Kirigami.Units.gridUnit * 20

	GeoLocationMap {
		id: map
		zoomLevelFactor: 0.2
		gesture.flickDeceleration: 3000
		anchors.fill: parent
		onCenterChanged: {
			if (!locationFollowingButton.checked) {
				root.selectedGeoLocation = center
			}
		}
		currentPositionSource.onPositionChanged: {
			if (position.coordinate.isValid && locationFollowingButton.checked) {
				root.selectedGeoLocation = position.coordinate
				map.center = root.selectedGeoLocation
			}
		}

		ZoomSlider {
			id: zoomSlider
			from: parent.minimumZoomLevel
			to: parent.maximumZoomLevel
			value: parent.zoomLevel
			width: parent.width - Kirigami.Units.gridUnit * 16
			onValueChanged: parent.zoomLevel = value
		}

		Controls.RoundButton {
			id: locationFollowingButton
			enabled: parent.currentPositionSource.supportedPositioningMethods !== PositionSource.NoPositioningMethods
			checked: true
			icon {
				name: checked
					  ? MediaUtils.newMediaIconName(Enums.MessageType.MessageGeoLocation)
					  : 'find-location-symbolic'
			}
			anchors {
				left: zoomSlider.right
				verticalCenter: zoomSlider.verticalCenter
				leftMargin: Kirigami.Units.largeSpacing
			}
			onCheckedChanged: {
				if (checked) {
					root.selectedGeoLocation = parent.currentPositionSource.position.coordinate
					parent.center = root.selectedGeoLocation
				} else {
					root.selectedGeoLocation = parent.center
				}
			}
		}

		Controls.RoundButton {
			icon.name: "mail-send-symbolic"
			anchors {
				left: locationFollowingButton.right
				verticalCenter: locationFollowingButton.verticalCenter
				leftMargin: Kirigami.Units.largeSpacing
			}
			onClicked: {
				root.messageComposition.body = Utils.geoUri(parent.center)
				root.messageComposition.send()
				root.close()
			}
			Component.onCompleted: forceActiveFocus()
		}

		GeocodeModel {
			plugin: map.plugin
			autoUpdate: false
			onCountChanged: {
				if(count > 0 && root.selectedGeoLocation === undefined) {
					const coordinate = get(0).coordinate
					root.selectedGeoLocation = coordinate
					map.center = coordinate
				}
			}
			Component.onCompleted: {
				query = Utils.systemCountryCode()
				update()
			}
		}
	}
}
