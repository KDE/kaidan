// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

MediaButton {
	id: root
	contentItem: Loader {
		sourceComponent: parent.enabled ? icon : busyIndicator

		Component {
			id: icon

			Kirigami.Icon {
				source: root.iconSource
			}
		}

		Component {
			id: busyIndicator

			Controls.BusyIndicator {}
		}
	}
}
