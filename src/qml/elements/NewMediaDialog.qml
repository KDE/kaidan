// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtMultimedia
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

Dialog {
	id: root

	default property alias __data: contentArea.data
	property MessageComposition messageComposition
	property alias captureSession: captureSession
	property alias shutterRelease: shutterRelease

	preferredHeight: Kirigami.Units.gridUnit * 19
	onClosed: {
		if (captureSession.camera) {
			let recording = false
			let actualLocation
			const recorder = captureSession.recorder

			if (recorder && recorder.recorderState === MediaRecorder.RecordingState) {
				recording = true
				actualLocation = recorder.actualLocation
			}

			// Remove a video file if the dialog is closed before manually stopping the recording.
			if (recording) {
				MediaUtils.deleteFile(actualLocation)
			}
		}
	}

	Rectangle {
		id: contentArea
		anchors.fill: parent

		VideoOutput {
			id: videoOutput
			fillMode: VideoOutput.PreserveAspectCrop
			anchors.fill: parent
		}

		CameraStatus {
			id: cameraStatusArea
			visible: !captureSession.camera
			anchors.fill: parent
		}

		ShutterRelease {
			id: shutterRelease
			visible: captureSession.camera
			anchors {
				horizontalCenter: zoomSlider.horizontalCenter
				bottom: zoomSlider.top
				bottomMargin: Kirigami.Units.smallSpacing * 3
			}
			Keys.onPressed: {
				if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
					clicked()
				}
			}
		}

		ZoomSlider {
			id: zoomSlider
			visible: captureSession.camera
			// TODO: Make the slider not stop to zoom when the handle reached a quarter of the total width
			to: captureSession.camera ? captureSession.camera.maximumZoomFactor : 0
		}
	}

	CaptureSession {
		id: captureSession
		camera: cameraLoader.item
		videoOutput: videoOutput
	}

	Loader {
		id: cameraLoader
		sourceComponent: mediaDevices.videoInputs.length ? cameraComponent : null

		Component {
			id: cameraComponent

			Camera {
				active: true
				focusMode: Camera.FocusModeAuto
				flashMode: Camera.FlashAuto
				zoomFactor: zoomSlider.value
			}
		}
	}

	MediaDevices {
		id: mediaDevices
	}

	function addFile(localFileUrl) {
		messageComposition.fileSelectionModel.addFile(localFileUrl, true)
		close()
	}
}
