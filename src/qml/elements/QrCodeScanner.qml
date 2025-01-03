// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import QtQuick.Controls as Controls
import QtMultimedia
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

/**
 * This is a scanner for QR codes which displays the camera input.
 */
Item {
	id: root

	property bool cameraEnabled: false
	property alias camera: reloadingCameraTimer.object
	// property alias filter: filter
	property alias zoomSlider: zoomSlider
	property bool cornersRounded: true

	// video output from the camera which is shown on the screen and decoded by a filter
	VideoOutput {
		id: videoOutput
		fillMode: VideoOutput.PreserveAspectCrop
		// filters: [filter]
		layer.enabled: true
		layer.effect: MultiEffect {
			maskSource: ShaderEffectSource {
				sourceItem: Rectangle {
					radius: cameraStatusArea.radius
					width: Math.min(videoOutput.width, videoOutput.height)
					height: width
					anchors.centerIn: parent
				}
			}
		}
		anchors.fill: parent
	}

	ZoomSlider {
		id: zoomSlider
		visible: root.camera.cameraStatus === Camera.ActiveStatus
	}

	CameraStatus {
		id: cameraStatusArea
		availability: root.camera.availability
		status: root.camera.cameraStatus
		radius: root.cornersRounded ? relativeRoundedCornersRadius(width, height) : 0
		anchors.fill: parent
	}

	// // filter which converts the video frames to images and decodes a containing QR code
	// QrCodeScannerFilter {
	// 	id: filter

	// 	onUnsupportedFormatReceived: {
	// 		popLayer()
	// 		passiveNotification(qsTr("The camera format '%1' is not supported.").arg(format))
	// 	}
	// }

	RecreationTimer {
		id: reloadingCameraTimer
		objectComponent: Camera {
			// Show camera input if this page is visible and the camera enabled.
			// cameraState: {
			// 	if (root.visible && root.cameraEnabled) {
			// 		return Camera.ActiveState
			// 	}

			// 	return Camera.LoadedState
			// }
			active: root.visible && root.cameraEnabled
			zoomFactor: zoomSlider.value
			onErrorOccurred: reloadingCameraTimer.start()
			Component.onCompleted: {
				if (availability !== Camera.Available) {
					reloadingCameraTimer.start()
					return
				}

				if (focus.supportedFocusModes.includes(Camera.FocusContinuous)) {
					focus.focusMode = Camera.FocusContinuous
				}

				if (focus.supportedFocusPointModes.includes(Camera.FocusPointCenter)) {
					focus.focusPointMode = Camera.FocusPointCenter
				}

				// filter.setCameraDefaultVideoFormat(this)
			}
		}
	}
}
