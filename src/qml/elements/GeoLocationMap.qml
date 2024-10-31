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

Map {
	id: root

	property real zoomLevelFactor: 0.85
	property alias currentPositionSource: currentPositionSource
	property alias userPositionMarker: userPositionMarker

	// The product is rounded for sharper tiles.
	zoomLevel: Math.max(minimumZoomLevel, Math.round(maximumZoomLevel * zoomLevelFactor))
	plugin: Plugin {
		name: "osm"
		PluginParameter {
			name: "osm.useragent"
			value: Utils.osmUserAgent()
		}
		PluginParameter {
			name: "osm.mapping.providersrepository.address"
			value: "https://autoconfig.kde.org/qtlocation/"
		}
	}
	copyrightsVisible: false
	onErrorChanged: {
		if (error !== Location.Map.NoError) {
			console.log("***", errorString)
		}
	}
	Keys.onPressed: {
		if (event.key === Qt.Key_Plus) {
			zoomLevel++
		} else if (event.key === Qt.Key_Minus) {
			zoomLevel--
		}
	}

	PositionSource {
		id: currentPositionSource
		onPositionChanged: {
			if (!position.coordinate.isValid) {
				console.log('***', 'Cannot locate this device')
			}
		}
		onSourceErrorChanged: {
			if (sourceError !== PositionSource.NoError) {
				console.log("***", sourceError)
				stop()
			}
		}
		onUpdateTimeout: {
			console.log("***", "Position lookup timeout")
		}
	}

	MapQuickItem {
		id: currentPositionMarker
		coordinate: currentPositionSource.position.coordinate
		anchorPoint: Qt.point(sourceItem.width / 2, sourceItem.height / 2)
		sourceItem: Rectangle {
			color: Kirigami.Theme.neutralTextColor
			radius: height / 2
			border {
				width: 2
				color: Qt.lighter(color)
			}
			width: Kirigami.Units.iconSizes.medium
			height: width
		}
	}

	MapQuickItem {
		id: userPositionMarker
		coordinate: parent.center
		anchorPoint: Qt.point(sourceItem.width / 2, sourceItem.height)
		sourceItem: Kirigami.Icon {
			source: MediaUtils.newMediaIconName(Enums.MessageType.MessageGeoLocation)
			color: Kirigami.Theme.activeTextColor
			smooth: true
			width: Kirigami.Units.iconSizes.large
			height: width
		}
	}

	ClickableText {
		text: qsTr("© OpenStreetMap")
		horizontalAlignment: Text.AlignHCenter
		scaleFactor: 0.9
		padding: Kirigami.Units.smallSpacing
		background: Rectangle {
			color: primaryBackgroundColor
			opacity: 0.8
			radius: roundedCornersRadius
		}
		anchors {
			left: parent.left
			bottom: parent.bottom
			leftMargin: Kirigami.Units.smallSpacing
			bottomMargin: anchors.leftMargin
		}
		onClicked: Qt.openUrlExternally("https://www.openstreetmap.org/copyright")
	}
}
