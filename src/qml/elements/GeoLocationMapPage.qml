// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami

Kirigami.Page {
	id: root
	property alias coordinate: map.center

	title: qsTr("Show location")
	padding: 0

	GeoLocationMap {
		id: map
		zoomLevelFactor: 0.55
		anchors.fill: parent
		Component.onCompleted: userPositionMarker.coordinate = center

		ZoomSlider {
			from: parent.minimumZoomLevel
			to: parent.maximumZoomLevel
			value: parent.zoomLevel
			width: parent.width - Kirigami.Units.gridUnit * 16
			onValueChanged: parent.zoomLevel = value
		}
	}
}
