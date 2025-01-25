// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

/**
 * This is an indicator for an ongoing audio/video recording.
 */
RowLayout {
	required property int duration

	Behavior on opacity {
		NumberAnimation {}
	}

	Kirigami.Icon {
		source: "media-record-symbolic"
		color: Kirigami.Theme.negativeTextColor
		isMask: true
		Layout.preferredWidth: Kirigami.Units.iconSizes.small

		Timer {
			interval: Kirigami.Units.veryLongDuration
			repeat: true
			running: true
			onTriggered: parent.opacity = 1 - parent.opacity
		}
	}

	Controls.Label {
		text: MediaUtils.prettyDuration(parent.duration)
	}
}
