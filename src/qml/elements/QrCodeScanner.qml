// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls
import QtMultimedia 5.14
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

/**
 * This is a scanner for QR codes which displays the camera input.
 */
Item {
	id: root
	property bool cameraEnabled: false
	property Camera camera
	property alias filter: filter
	property bool cornersRounded: true

	Component.onCompleted: cameraComponent.createObject()

	Component {
		id: cameraComponent

		// camera with continuous focus in the center of the video
		Camera {
			// Show camera input if this page is visible and the camera enabled.
			cameraState: {
				if (visible && cameraEnabled) {
					return Camera.ActiveState
				}

				return Camera.LoadedState
			}
			digitalZoom: zoomSlider.value
			onError: reloadingCameraTimer.start()
			Component.onCompleted: {
				root.camera = this

				if (availability !== Camera.Available) {
					reloadingCameraTimer.start()
					return
				}

				// Setting the properties here avoids triggering warnings multiple times if the
				// desired features are unsupported by the camera device and "reloadingCameraTimer"
				// started because of an unavailabe camera device.
				focus.focusMode = Camera.FocusContinuous
				focus.focusPointMode = Camera.FocusPointCenter

				filter.setCameraDefaultVideoFormat(this)
			}
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
		fillMode: VideoOutput.PreserveAspectCrop
		source: root.camera
		autoOrientation: true
		filters: [filter]
		anchors.fill: parent

		Rectangle {
			color: "transparent"
			border.color: secondaryBackgroundColor
			border.width: radius * 0.3
			radius: cameraStatusArea.radius * 1.5
			anchors.fill: parent
			anchors.margins: - border.width
		}

		Rectangle {
			color: primaryBackgroundColor
			opacity: 0.9
			radius: relativeRoundedCornersRadius(width, height) * 2
			width: parent.width - Kirigami.Units.largeSpacing * 4
			height: Kirigami.Units.largeSpacing * 4
			anchors.bottom: parent.bottom
			anchors.bottomMargin: Kirigami.Units.largeSpacing * 2
			anchors.horizontalCenter: parent.horizontalCenter

			Controls.Slider {
				id: zoomSlider
				value: 1
				from: 1
				to: 3
				width: parent.width - Kirigami.Units.largeSpacing * 3
				anchors.centerIn: parent
			}
		}
	}

	// hint for camera status
	Rectangle {
		id: cameraStatusArea
		visible: root.cameraEnabled && cameraStatusText.text.length
		color: primaryBackgroundColor
		radius: root.cornersRounded ? relativeRoundedCornersRadius(width, height) : 0
		anchors.fill: parent

		ColumnLayout {
			anchors.fill: parent

			// This placeholder is needed to position the icon and text in the center without additional
			// spacing between them while keeping the text's end being elided.
			// The spacing is multiplied by 3 for the spacings between all items including the
			// placeholders themselves.
			Item {
				Layout.preferredHeight: (parent.height - parent.spacing * 3 - cameraStatusIcon.height - cameraStatusText.height) / 2
				Layout.fillHeight: true
			}

			Controls.BusyIndicator {
				visible: !cameraStatusIcon.visible
				Layout.preferredWidth: cameraStatusIcon.Layout.preferredWidth
				Layout.preferredHeight: Layout.preferredWidth
				Layout.alignment: Qt.AlignHCenter
			}

			Kirigami.Icon {
				id: cameraStatusIcon
				source: "camera-disabled-symbolic"
				fallback: "camera-off-symbolic"
				color: Kirigami.Theme.neutralTextColor
				visible: root.camera.cameraStatus !== Camera.StartingStatus
				Layout.preferredWidth: Kirigami.Units.iconSizes.enormous
				Layout.preferredHeight: Layout.preferredWidth
				Layout.fillHeight: true
				Layout.alignment: Qt.AlignHCenter
			}

			Kirigami.Heading {
				id: cameraStatusText
				text: {
					if (root.camera.cameraStatus === Camera.StartingStatus) {
						return qsTr("Loading camera…")
					}

					switch (root.camera.availability) {
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
				color: root.camera.cameraStatus === Camera.StartingStatus ? Kirigami.Theme.textColor : Kirigami.Theme.neutralTextColor
				wrapMode: Text.Wrap
				elide: Text.ElideRight
				Layout.fillWidth: true
				Layout.fillHeight: true
				horizontalAlignment: Text.AlignHCenter
			}

			// This placeholder is needed to position the icon and text in the center without additional
			// spacing between them while keeping the text's end being elided.
			// The spacing is multiplied by 3 for the spacings between all items including the
			// placeholders themselves.
			Item {
				Layout.preferredHeight: (parent.height - parent.spacing * 3 - cameraStatusIcon.height - cameraStatusText.height) / 2
				Layout.fillHeight: true
			}
		}
	}

	// This timer is used to reload the camera device in case it is not available at the time of
	// creation.
	// Reloading is needed if the camera device is not plugged in or disabled.
	// That approach ensures that a camera device is even detected again after plugging it out and
	// plugging it in while the scanner is used.
	//
	Timer {
		id: reloadingCameraTimer
		interval: Kirigami.Units.veryLongDuration
		onTriggered: {
			root.camera.destroy()
			cameraComponent.createObject()
		}
	}
}
