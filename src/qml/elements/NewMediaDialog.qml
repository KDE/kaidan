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
	property alias cameraStatusArea: cameraStatusArea
	property alias shutterRelease: shutterRelease
	property alias zoomSlider: zoomSlider
	property alias camera: reloadingCameraTimer.object

	preferredHeight: Kirigami.Units.gridUnit * 19
	onClosed: {
		const recorderStatus = camera.videoRecorder.recorderStatus
		const actualLocation = camera.videoRecorder.actualLocation

		// The camera needs to be explicitly destroyed.
		// Otherwise, the resource would stay busy.
		camera.destroy()

		// Remove a video file if the dialog is closed before manually stopping the recording.
		if (recorderStatus === CameraRecorder.RecordingStatus) {
			MediaUtils.deleteFile(actualLocation)
		}
	}

	Rectangle {
		id: contentArea
		anchors.fill: parent

		VideoOutput {
			source: camera
			fillMode: VideoOutput.PreserveAspectCrop
			autoOrientation: true
			focus: visible // to receive focus and capture key events when visible
			anchors.fill: parent
		}

		CameraStatus {
			id: cameraStatusArea
			availability: root.camera.availability
			status: root.camera.cameraStatus
			radius: root.cornersRounded ? relativeRoundedCornersRadius(width, height) : 0
			anchors.fill: parent
		}

		ShutterRelease {
			id: shutterRelease
			visible: !cameraStatusArea.visible
			anchors {
				horizontalCenter: root.zoomSlider.horizontalCenter
				bottom: root.zoomSlider.top
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
			visible: camera.cameraStatus === Camera.ActiveStatus
		}
	}

	RecreationTimer {
		id: reloadingCameraTimer
		objectComponent: Camera {
			digitalZoom: zoomSlider.value
			onError: reloadingCameraTimer.start()
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
			}
		}
	}

	function addFile(localFileUrl) {
		messageComposition.fileSelectionModel.addFile(localFileUrl, true)
		close()
	}
}
