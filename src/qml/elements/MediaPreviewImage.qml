// SPDX-FileCopyrightText: 2018 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * This element is used in the @see SendMediaSheet to display information about a selected image to
 * the user. It just displays the image in a rectangle.
 */

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls
import org.kde.kirigami 2.19 as Kirigami

MediaPreview {
	Layout.preferredHeight: message ? messageSize : Kirigami.Units.gridUnit * 18
	Layout.preferredWidth: Kirigami.Units.gridUnit * 32
	Layout.maximumWidth: message ? messageSize : -1

	Image {
		id: image

		fillMode: Image.PreserveAspectFit
		asynchronous: true // image might be very large
		mipmap: true
		source: root.mediaSource

		anchors {
			fill: parent
		}

		MouseArea {
			enabled: root.showOpenButton

			anchors {
				fill: parent
			}

			onClicked: Qt.openUrlExternally(root.mediaSource)
		}
	}
}
