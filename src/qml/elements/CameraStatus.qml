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

	color: primaryBackgroundColor

	ColumnLayout {
		anchors.fill: parent
		anchors.margins: Kirigami.Units.largeSpacing

		// The layout is needed to position the icon and text in the center without additional
		// spacing between them while keeping the text's end being elided.
		ColumnLayout {
			Kirigami.Icon {
				id: cameraStatusIcon
				source: "camera-disabled-symbolic"
				fallback: "camera-off"
				Layout.maximumWidth: Kirigami.Units.iconSizes.enormous
				Layout.maximumHeight: Layout.maximumWidth
				Layout.fillWidth: true
				Layout.fillHeight: true
				Layout.alignment: Qt.AlignHCenter
			}

			Kirigami.Heading {
				text: qsTr("No camera available")
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
