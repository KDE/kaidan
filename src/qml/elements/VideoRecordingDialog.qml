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
		recorder: MediaRecorder {
			onRecorderStateChanged: {
				const actualLocation = root.captureSession.recorder.actualLocation

				if (root.captureSession.recorder.recorderState !== MediaRecorder.RecordingState && actualLocation) {
					root.addFile(actualLocation)
				}
			}
		}
	}
	shutterRelease {
		iconSource: captureSession.recorder.recorderState === MediaRecorder.RecordingState ? "media-playback-stop-symbolic" : "camera-video-symbolic"
		onClicked: {
			if (captureSession.recorder.recorderState === MediaRecorder.RecordingState) {
				root.savingCapturedData = true
				captureSession.recorder.stop()
			} else {
				captureSession.recorder.outputLocation = MediaUtils.newVideoFileUrl()
				captureSession.recorder.record()
			}
		}
	}

	// Recording indicator
	Controls.Control {
		opacity: root.captureSession.recorder.recorderState === MediaRecorder.RecordingState && !root.savingCapturedData ? 0.9 : 0
		topPadding: Kirigami.Units.smallSpacing
		bottomPadding: topPadding
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

		Behavior on opacity {
			NumberAnimation {}
		}
	}
}
