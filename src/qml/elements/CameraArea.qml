// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtMultimedia
import org.kde.kirigami as Kirigami

/**
 * This is an area containing video output that is mirrored on desktop devices.
 */
Item {
	property alias output: output

	// Mirror the video output on desktop devices.
	transform: Rotation {
		origin.x: width / 2
		axis {
			x: 0
			y: 1
			z: 0
		}
		angle: Kirigami.Settings.isMobile ? 0 : 180
	}

	// Video output from the camera which is shown on the screen.
	VideoOutput {
		id: output
		fillMode: VideoOutput.PreserveAspectCrop
		anchors.fill: parent
	}
}
