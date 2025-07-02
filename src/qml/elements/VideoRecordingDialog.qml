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
	captureSession {
		audioInput: AudioInput {}
		recorder: MediaRecorder {}
	}
	shutterRelease {
		iconSource: captureSession.recorder.recorderState === MediaRecorder.RecordingState ? "media-playback-stop-symbolic" : "camera-video-symbolic"
		onClicked: {
			if (captureSession.recorder.recorderState === MediaRecorder.RecordingState) {
				captureSession.recorder.stop()
			} else {
				captureSession.recorder.outputLocation = MediaUtils.newVideoFileUrl()
				captureSession.recorder.record()
			}
		}
	}

	// Recording indicator
	Controls.Control {
		visible: root.captureSession.recorder.recorderState === MediaRecorder.RecordingState
		opacity: visible ? 1 : 0
		topPadding: 0
		bottomPadding: topPadding
		Behavior on opacity {
			NumberAnimation {}
		}
		background: Kirigami.ShadowedRectangle {
			color: primaryBackgroundColor
			opacity: 0.9
			radius: height / 2
			shadow.color: Qt.darker(color, 1.2)
			shadow.size: 4
		}
		contentItem: RecordingIndicator {
			duration: root.captureSession.recorder.duration
		}
		anchors {
			right: root.shutterRelease.left
			rightMargin: Kirigami.Units.largeSpacing
			verticalCenter: root.shutterRelease.verticalCenter
		}
	}

	Connections {
		target: root.captureSession.recorder
		ignoreUnknownSignals: true

		function onRecorderStateChanged() {
			const actualLocation = root.captureSession.recorder.actualLocation

			if (root.captureSession.recorder.recorderState !== MediaRecorder.RecordingState && actualLocation) {
				root.addFile(actualLocation)
			}
		}
	}
}
