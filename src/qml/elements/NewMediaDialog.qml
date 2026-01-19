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
	property FileSelectionModel fileSelectionModel
	property alias captureSession: captureSession
	property alias shutterRelease: shutterRelease
	property alias zoomSlider: zoomSlider
	property bool savingCapturedData: false

	preferredWidth: applicationWindow().width - Kirigami.Units.gridUnit * 3
	preferredHeight: applicationWindow().height - Kirigami.Units.gridUnit * 6
	maximumWidth: Kirigami.Units.gridUnit * 60
	maximumHeight: Kirigami.Units.gridUnit * 40
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
		width: root.width
		height: root.height - root.header.height - Kirigami.Units.cornerRadius / 2

		CameraArea {
			id: cameraArea
			output {
				layer.enabled: GraphicsInfo.api !== GraphicsInfo.Software
				layer.effect: Kirigami.ShadowedTexture {
					corners {
						bottomLeftRadius: Kirigami.Units.cornerRadius
						bottomRightRadius: Kirigami.Units.cornerRadius
					}
				}
			}
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
			enabled: !root.savingCapturedData
			anchors {
				horizontalCenter: zoomSlider.horizontalCenter
				bottom: zoomSlider.top
				bottomMargin: zoomSlider.opacity > 0.5 ? Kirigami.Units.smallSpacing * 3 : - Kirigami.Units.smallSpacing * 5 - zoomSlider.height

				Behavior on bottomMargin {
					SmoothedAnimation {}
				}
			}
			Keys.onPressed: {
				if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
					clicked()
				}
			}
		}

		ZoomSlider {
			id: zoomSlider
			opacity: captureSession.camera && !root.savingCapturedData ? 1 : 0
			// TODO: Make the slider not stop to zoom when the handle reached a quarter of the total width
			to: captureSession.camera ? captureSession.camera.maximumZoomFactor : 0

			Behavior on opacity {
				NumberAnimation {}
			}
		}
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
		fileSelectionModel.addFile(localFileUrl, true)
		close()
	}
}
