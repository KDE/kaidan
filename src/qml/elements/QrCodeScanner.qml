// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import QtMultimedia
import org.kde.kirigami as Kirigami
import org.kde.prison.scanner as Prison

import im.kaidan.kaidan

/**
 * This is a scanner for QR codes which displays the camera input.
 */
Item {
	id: root

	property bool cameraEnabled: false
	property alias filter: filter
	property bool acceptingResult: true
	property alias zoomSlider: zoomSlider
	property bool cornersRounded: true

	// timer to accept the result again after a QR code was scanned containing content that cannot be processed
	property Timer resetAcceptingResultTimer: Timer {
		interval: Kirigami.Units.veryLongDuration * 4
		onTriggered: root.acceptingResult = true
	}

	CameraArea {
		id: cameraArea
		output {
			layer.enabled: GraphicsInfo.api !== GraphicsInfo.Software
			layer.effect: Kirigami.ShadowedTexture {
				radius: cameraStatusArea.radius
			}
		}
		anchors.fill: parent
	}

	CameraStatus {
		id: cameraStatusArea
		visible: !captureSession.camera
		radius: root.cornersRounded ? relativeRoundedCornersRadius(width, height) : 0
		anchors.fill: parent
	}

	ZoomSlider {
		id: zoomSlider
		visible: captureSession.camera
		// TODO: Make the slider not stop to zoom when the handle reached a quarter of the total width
		to: captureSession.camera ? captureSession.camera.maximumZoomFactor : 0
	}

	// filter which converts the video frames to images and decodes a containing QR code
	Prison.VideoScanner {
		id: filter
		formats: Prison.Format.QRCode
		videoSink: cameraArea.output.videoSink
	}

	CaptureSession {
		id: captureSession
		camera: cameraLoader.item
		videoOutput: cameraArea.output
	}

	Loader {
		id: cameraLoader
		sourceComponent: mediaDevices.videoInputs.length ? cameraComponent : null

		Component {
			id: cameraComponent

			Camera {
				active: root.visible && root.cameraEnabled
				focusMode: Camera.FocusModeAuto
				flashMode: Camera.FlashAuto
				zoomFactor: zoomSlider.value
			}
		}
	}

	MediaDevices {
		id: mediaDevices
	}
}
