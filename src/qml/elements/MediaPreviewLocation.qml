// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * This element is used in the @see SendMediaSheet to display information about a shared location to
 * the user. It just displays the map in a rectangle.
 */

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtLocation 5.14 as Location
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0
import MediaUtils 0.1

MediaPreview {
	id: root

	Layout.preferredHeight: message ? messageSize : Kirigami.Units.gridUnit * 18
	Layout.preferredWidth: Kirigami.Units.gridUnit * 32
	Layout.maximumWidth: message ? messageSize : -1

	ColumnLayout {
		anchors {
			fill: parent
		}

		Location.Map {
			id: map

			zoomLevel: (maximumZoomLevel - minimumZoomLevel) / 1.2
			center: MediaUtilsInstance.locationCoordinate(root.mediaSource)
			copyrightsVisible: false

			plugin: Location.Plugin {
				name: "osm"
				Location.PluginParameter {
					name: "osm.useragent"
					value: Utils.osmUserAgent()
				}
				Location.PluginParameter {
					name: "osm.mapping.providersrepository.address"
					value: "https://autoconfig.kde.org/qtlocation/"
				}
			}

			gesture {
				enabled: false
			}

			Layout.fillHeight: true
			Layout.fillWidth: true

			onErrorChanged: {
				if (map.error !== Location.Map.NoError) {
					console.log("***", map.errorString)
				}
			}

			Location.MapQuickItem {
				id: positionMarker

				coordinate: map.center
				anchorPoint: Qt.point(sourceItem.width / 2, sourceItem.height)

				sourceItem: Kirigami.Icon {
					source: MediaUtilsInstance.newMediaIconName(Enums.MessageType.MessageGeoLocation)
					height: 48
					width: height
					color: "#e41e25"
					smooth: true
				}
			}

			MouseArea {
				enabled: root.showOpenButton

				anchors {
					fill: parent
				}

				onClicked: {
					if (!Qt.openUrlExternally(root.message.messageBody)) {
						Qt.openUrlExternally('https://www.openstreetmap.org/?mlat=%1&mlon=%2&zoom=18&layers=M'
											 .arg(root.geoLocation.latitude).arg(root.geoLocation.longitude))
					}
				}
			}
		}
	}
}
