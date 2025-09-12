// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami
import QtMultimedia

import im.kaidan.kaidan

import "elements"

/**
 * This page is used for audio and video calls.
 */
Kirigami.Page {
	id: root

	property Account account
	property string chatJid
	property string chatName
	property bool cameraActive: false

	title: qsTr("Call")
	padding: 0
	Component.onCompleted: account.callController.call(chatJid, !cameraActive)
	// Component.onDestruction: account.callController.hangUp()

	ScalableText {
		text: root.chatName
		scaleFactor: 5
		anchors.horizontalCenter: avatar.horizontalCenter
		anchors.bottom: avatar.top
		anchors.bottomMargin: Kirigami.Units.gridUnit
	}

	Avatar {
		id: avatar
		account: root.account
		jid: root.chatJid
		name: root.chatName
		cache: true
		anchors {
			fill: parent
			margins: parent.width * 0.2
		}
	}

	VideoOutput {
		id: contactVideoOutput
		fillMode: VideoOutput.PreserveAspectCrop
		anchors.fill: parent
	}

	CameraArea {
		id: cameraArea
		visible: root.cameraActive
		width: {
			if (output.sourceRect.width > output.sourceRect.height) {
				if (root.width > root.height) {
					return root.height * 0.18
				}

				return root.width * 0.18
			}

			if (root.width > root.height) {
				return root.height * 0.1
			}

			return root.width * 0.1
		}
		height: {
			if (output.sourceRect.width > output.sourceRect.height) {
				if (root.width > root.height) {
					return root.height * 0.1
				}

				return root.width * 0.1
			}

			if (root.width > root.height) {
				return root.height * 0.18
			}

			return root.width * 0.18
		}
		layer.enabled: GraphicsInfo.api !== GraphicsInfo.Software
		layer.effect: Kirigami.ShadowedTexture {
			radius: relativeRoundedCornersRadius(width, height)
		}
		anchors {
			top: parent.top
			right: parent.right
			topMargin: Kirigami.Units.largeSpacing
			rightMargin: Kirigami.Units.largeSpacing
		}
	}

	CameraStatus {
		id: cameraStatusArea
		visible: !captureSession.camera
		anchors.fill: parent
	}

	MediaButton {
		iconSource: "audio-input-microphone-symbolic"
		iconSize: Kirigami.Units.iconSizes.smallMedium
		iconColor: captureSession.audioInput.muted ? Kirigami.Theme.neutralTextColor : Kirigami.Theme.textColor
		anchors {
			right: callStopButton.left
			rightMargin: Kirigami.Units.largeSpacing
			verticalCenter: callStopButton.verticalCenter
		}
		onClicked: captureSession.audioInput.muted = !captureSession.audioInput.muted
	}

	MediaButton {
		id: callStopButton
		iconSource: "call-stop-symbolic"
		iconColor: Kirigami.Theme.negativeTextColor
		anchors {
			horizontalCenter: parent.horizontalCenter
			bottom: parent.bottom
			bottomMargin: Kirigami.Units.smallSpacing * 3
		}
		onClicked: {
			account.callController.hangUp()
			popLayer()
		}
	}

	MediaButton {
		iconSource: "camera-video-symbolic"
		iconSize: Kirigami.Units.iconSizes.smallMedium
		iconColor: root.cameraActive ? Kirigami.Theme.textColor : Kirigami.Theme.neutralTextColor
		anchors {
			left: callStopButton.right
			leftMargin: Kirigami.Units.largeSpacing
			verticalCenter: callStopButton.verticalCenter
		}
		onClicked: root.cameraActive = !root.cameraActive
	}

	CaptureSession {
		id: captureSession
		camera: cameraLoader.item
		audioInput: AudioInput {}
		videoOutput: cameraArea.output
	}

	CaptureSession {
		id: contactCaptureSession
		camera: cameraLoader.item
		videoOutput: contactVideoOutput
	}

	Loader {
		id: cameraLoader
		sourceComponent: mediaDevices.videoInputs.length ? cameraComponent : null

		Component {
			id: cameraComponent

			Camera {
				active: root.cameraActive
				focusMode: Camera.FocusModeAuto
				flashMode: Camera.FlashAuto
			}
		}
	}

	MediaDevices {
		id: mediaDevices
	}
}
