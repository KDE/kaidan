// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtMultimedia 5.14
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

/**
 * This is a scanner for QR codes which displays the camera input.
 */
Item {
	property bool cameraEnabled: false

	property alias camera: camera
	property alias filter: filter

	// camera with continuous focus in the center of the video
	Camera {
		id: camera
		focus.focusMode: Camera.FocusContinuous
		focus.focusPointMode: Camera.FocusPointCenter

		// Show camera input if this page is visible and the camera enabled.
		cameraState: {
			if (visible && cameraEnabled)
				return Camera.ActiveState
			return Camera.LoadedState
		}

		Component.onCompleted: {
			filter.setCameraDefaultVideoFormat(camera);
		}
	}

	// filter which converts the video frames to images and decodes a containing QR code
	QrCodeScannerFilter {
		id: filter

		onUnsupportedFormatReceived: {
			pageStack.layers.pop()
			passiveNotification(qsTr("The camera format '%1' is not supported.").arg(format))
		}
	}

	// video output from the camera which is shown on the screen and decoded by a filter
	VideoOutput {
		anchors.fill: parent
		fillMode: VideoOutput.PreserveAspectCrop
		source: camera
		autoOrientation: true
		filters: [filter]
	}

	// hint for camera issues
	Kirigami.InlineMessage {
		visible: cameraEnabled && text !== ""
		icon.source: "camera-video-symbolic"
		type: Kirigami.MessageType.Warning
		anchors.centerIn: parent
		width: Math.min(largeButtonWidth, parent.width)
		height: Math.min(60, parent.height)

		text: {
			switch (camera.availability) {
			case Camera.Unavailable:
			case Camera.ResourceMissing:
				// message to be shown if no camera can be found
				return qsTr("There is no camera available.")
			case Camera.Busy:
				// message to be shown if the found camera is not usable
				return qsTr("Your camera is busy.\nTry to close other applications using the camera.")
			default:
				// no message if no issue could be found
				return ""
			}
		}
	}
}
