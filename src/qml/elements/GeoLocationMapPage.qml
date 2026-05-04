// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import org.kde.kirigami as Kirigami

Kirigami.Page {
	id: root

	property alias map: map
	property alias coordinate: map.center

	title: qsTr("Show location")
	padding: 0

	GeoLocationInteractiveMap {
		id: map
		anchors.fill: parent
	}
}
