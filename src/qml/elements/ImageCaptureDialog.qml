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

	title: qsTr("Take a picture")
	shutterRelease {
		iconSource: "camera-photo-symbolic"
		enabled: camera.imageCapture.ready
		onClicked: camera.imageCapture.captureToLocation(MediaUtils.newImageFilePath())
	}

	Connections {
		target: root.camera.imageCapture
		ignoreUnknownSignals: true

		function onImageSaved(requestId, path) {
			root.addFile(MediaUtils.localFileUrl(path))
		}
	}
}
