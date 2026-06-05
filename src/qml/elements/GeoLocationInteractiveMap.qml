// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtPositioning
import QtLocation
import Qt.labs.animation
import org.kde.kirigami as Kirigami

GeoLocationMap {
	id: root

	property alias currentPositionSource: currentPositionSource
	property alias zoomSlider: zoomSlider

	Keys.onPressed: event => {
		switch(event.key) {
		case Qt.Key_Plus:
			zoomLevel = Math.min(maximumZoomLevel, zoomLevel + 1)
			break
		case Qt.Key_Minus:
			zoomLevel = Math.max(minimumZoomLevel, zoomLevel - 1)
			break
		}
	}

	MapQuickItem {
		id: currentPositionMarker
		coordinate: currentPositionSource.position.coordinate
		anchorPoint: Qt.point(sourceItem.width / 2, sourceItem.height / 2)
		sourceItem: Rectangle {
			color: Kirigami.Theme.activeTextColor
			radius: height / 2
			border {
				width: 2
				color: Qt.lighter(color)
			}
			width: Kirigami.Units.iconSizes.medium
			height: width
		}
	}

	ZoomSlider {
		id: zoomSlider
		from: map.minimumZoomLevel
		to: map.maximumZoomLevel
		value: map.zoomLevel
		width: map.width - Kirigami.Units.gridUnit * 16
		onMoved: map.zoomLevel = value
	}

	WheelHandler {
		property: "zoomLevel"
		rotationScale: 1/120
		acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
		onActiveChanged: {
			if (!active) {
				zoomLevelBoundaryRule.returnToBounds()
			}
		}
	}

	BoundaryRule on zoomLevel {
		id: zoomLevelBoundaryRule
		minimum: root.minimumZoomLevel
		maximum: root.maximumZoomLevel
		minimumOvershoot: 1
		maximumOvershoot: 1
		overshootFilter: BoundaryRule.Peak
	}

	DragHandler {
		target: null
		onTranslationChanged: (delta) => root.pan(-delta.x, -delta.y)
	}

	PositionSource {
		id: currentPositionSource
		onSourceErrorChanged: {
			if (sourceError !== PositionSource.NoError) {
				console.log("Could not locate this device: " + sourceError)
			}
		}
	}
}
