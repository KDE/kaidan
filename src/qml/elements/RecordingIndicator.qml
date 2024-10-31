// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

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
