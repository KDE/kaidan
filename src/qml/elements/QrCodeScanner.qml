/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2022 Kaidan developers and contributors
 *  (see the LICENSE file for a full list of copyright authors)
 *
 *  Kaidan is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  In addition, as a special exception, the author of Kaidan gives
 *  permission to link the code of its release with the OpenSSL
 *  project's "OpenSSL" library (or with modified versions of it that
 *  use the same license as the "OpenSSL" library), and distribute the
 *  linked executables. You must obey the GNU General Public License in
 *  all respects for all of the code used other than "OpenSSL". If you
 *  modify this file, you may extend this exception to your version of
 *  the file, but you are not obligated to do so.  If you do not wish to
 *  do so, delete this exception statement from your version.
 *
 *  Kaidan is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kaidan.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtMultimedia 5.14
import org.kde.kirigami 2.12 as Kirigami

import im.kaidan.kaidan 1.0

/**
 * This is a scanner for QR codes which displays the camera input.
 */
Item {
	property bool cameraEnabled: false

	property alias camera: camera
	property alias filter: filter

	// camera with continuous focus in the center of the video
	Camera {
		id: camera
		focus.focusMode: Camera.FocusContinuous
		focus.focusPointMode: Camera.FocusPointCenter

		// Show camera input if this page is visible and the camera enabled.
		cameraState: {
			if (visible && cameraEnabled)
				return Camera.ActiveState
			return Camera.LoadedState
		}

		Component.onCompleted: {
			filter.setCameraDefaultVideoFormat(camera);
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
		anchors.fill: parent
		fillMode: VideoOutput.PreserveAspectCrop
		source: camera
		autoOrientation: true
		filters: [filter]
	}

	// hint for camera issues
	Kirigami.InlineMessage {
		visible: cameraEnabled && text !== ""
		icon.source: "camera-video-symbolic"
		type: Kirigami.MessageType.Warning
		anchors.centerIn: parent
		width: Math.min(largeButtonWidth, parent.width)
		height: Math.min(60, parent.height)

		text: {
			switch (camera.availability) {
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
	}
}
