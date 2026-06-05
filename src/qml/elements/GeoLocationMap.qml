// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtPositioning
import QtLocation
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

Map {
	property alias positionMarker: positionMarker

	// The product is rounded for sharper tiles.
	zoomLevel: Math.max(minimumZoomLevel, Math.round(maximumZoomLevel * 0.85))
	minimumZoomLevel: 4
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
		if (error !== Map.NoError) {
			console.log(errorString)
		}
	}

	MapQuickItem {
		id: positionMarker
		anchorPoint: Qt.point(sourceItem.width / 2, sourceItem.height)
		sourceItem: Kirigami.Icon {
			source: MediaUtils.newMediaIconName(Enums.MessageType.MessageGeoLocation)
			smooth: true
			width: Kirigami.Units.iconSizes.large
			height: width
		}
	}

	TextButton {
		text: qsTr("© OpenStreetMap")
		font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 0.9
		padding: Kirigami.Units.smallSpacing
		anchors {
			left: parent.left
			bottom: parent.bottom
			leftMargin: Kirigami.Units.smallSpacing
			bottomMargin: anchors.leftMargin
		}
		onClicked: Qt.openUrlExternally("https://www.openstreetmap.org/copyright")
	}
}
