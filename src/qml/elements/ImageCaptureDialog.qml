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
	captureSession.imageCapture: ImageCapture {
		onImageSaved: (requestId, path) => {
			root.addFile(MediaUtils.localFileUrl(path))
		}
	}
	shutterRelease {
		z: imagePreview.z + 1
		iconSource: "camera-photo-symbolic"
		onClicked: {
			root.savingCapturedData = true
			captureSession.imageCapture.captureToFile(MediaUtils.localFilePath(MediaUtils.newImageFileUrl()))
		}
	}
	zoomSlider {
		z: imagePreview.z + 1
	}

	Image {
		id: imagePreview
		source: root.captureSession.imageCapture.preview
		fillMode: VideoOutput.PreserveAspectCrop
		visible: source.toString()
		layer.enabled: GraphicsInfo.api !== GraphicsInfo.Software
		layer.effect: Kirigami.ShadowedTexture {
			corners {
				bottomLeftRadius: Kirigami.Units.cornerRadius
				bottomRightRadius: Kirigami.Units.cornerRadius
			}
		}
		anchors.fill: parent
		// Mirror the image on desktop devices.
		transform: Rotation {
			origin.x: width / 2
			axis {
				x: 0
				y: 1
				z: 0
			}
			angle: Kirigami.Settings.isMobile ? 0 : 180
		}
	}
}
