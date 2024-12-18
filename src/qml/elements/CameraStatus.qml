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

/**
 * This is a hint for the status of a camera.
 */
Rectangle {
	id: root

	property int availability
	property int status

	visible: cameraStatusText.text
	color: primaryBackgroundColor
	radius: relativeRoundedCornersRadius(width, height)

	ColumnLayout {
		anchors.fill: parent
		anchors.margins: Kirigami.Units.largeSpacing

		// The layout is needed to position the icon and text in the center without additional
		// spacing between them while keeping the text's end being elided.
		ColumnLayout {
			Controls.BusyIndicator {
				visible: !cameraStatusIcon.visible
				Layout.maximumWidth: cameraStatusIcon.Layout.maximumWidth
				Layout.maximumHeight: Layout.maximumWidth
				Layout.fillWidth: true
				Layout.fillHeight: true
				Layout.alignment: Qt.AlignHCenter
			}

			Kirigami.Icon {
				id: cameraStatusIcon
				source: "camera-disabled-symbolic"
				fallback: "camera-off-symbolic"
				color: Kirigami.Theme.neutralTextColor
				visible: root.status !== Camera.StartingStatus
				Layout.maximumWidth: Kirigami.Units.iconSizes.enormous
				Layout.maximumHeight: Layout.maximumWidth
				Layout.fillWidth: true
				Layout.fillHeight: true
				Layout.alignment: Qt.AlignHCenter
			}

			Kirigami.Heading {
				id: cameraStatusText
				text: {
					if (root.status === Camera.StartingStatus) {
						return qsTr("Loading camera…")
					}

					switch (root.availability) {
					case Camera.Unavailable:
					case Camera.ResourceMissing:
						// message to be shown if no camera can be found
						return qsTr("No camera available")
					case Camera.Busy:
						// message to be shown if the found camera is not usable
						return qsTr("Camera busy\nTry to close other applications using the camera")
					default:
						// no message if no issue could be found
						return ""
					}
				}
				color: root.status === Camera.StartingStatus ? Kirigami.Theme.textColor : Kirigami.Theme.neutralTextColor
				wrapMode: Text.Wrap
				elide: Text.ElideRight
				horizontalAlignment: Text.AlignHCenter
				Layout.fillWidth: true
				// "Layout.fillHeight: true" cannot be used to position the icon and text in the
				// center without additional spacing between them while keeping the text's end
				// being elided.
				Layout.fillHeight: cameraStatusIcon.height + parent.spacing + implicitHeight > parent.parent.height
			}
		}
	}
}
