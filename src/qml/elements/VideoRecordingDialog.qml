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

NewMediaDialog {
	id: root

	title: qsTr("Record a video")
	shutterRelease {
		iconSource: camera.videoRecorder.recorderStatus === CameraRecorder.RecordingStatus ? "media-playback-stop-symbolic" : "camera-video-symbolic"
		enabled: camera.videoRecorder.recorderStatus > CameraRecorder.LoadingStatus && camera.videoRecorder.recorderStatus < CameraRecorder.FinalizingStatus
		onClicked: {
			if (camera.videoRecorder.recorderStatus === CameraRecorder.RecordingStatus) {
				camera.videoRecorder.stop()
			} else {
				camera.videoRecorder.outputLocation = MediaUtils.newVideoFilePath()
				camera.videoRecorder.record()
			}
		}
	}

	// Recording indicator
	Controls.Control {
		visible: root.camera.videoRecorder.recorderStatus === CameraRecorder.RecordingStatus
		opacity: visible ? 1 : 0
		topPadding: 0
		bottomPadding: topPadding
		Behavior on opacity {
			NumberAnimation {}
		}
		background: Kirigami.ShadowedRectangle  {
			color: primaryBackgroundColor
			opacity: 0.9
			radius: height / 2
			shadow.color: Qt.darker(color, 1.2)
			shadow.size: 4
		}
		contentItem: RecordingIndicator {
			duration: root.camera.videoRecorder.duration
		}
		anchors {
			right: root.shutterRelease.left
			rightMargin: Kirigami.Units.largeSpacing
			verticalCenter: root.shutterRelease.verticalCenter
		}
	}

	Connections {
		target: root

		function onCameraChanged() {
			root.camera.captureMode = Camera.CaptureVideo
		}
	}

	Connections {
		target: root.camera.videoRecorder
		ignoreUnknownSignals: true

		function onRecorderStatusChanged() {
			const actualLocation = root.camera.videoRecorder.actualLocation

			if (root.camera.videoRecorder.recorderStatus === CameraRecorder.LoadedStatus && actualLocation) {
				root.addFile(actualLocation)
			}
		}
	}
}
