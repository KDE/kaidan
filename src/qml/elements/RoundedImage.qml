// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2025 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

/**
 * Image with rounded corners.
 */
Image {
	id: root

	property real radius: relativeRoundedCornersRadius(width, height)
	readonly property alias hasAlphaChannel: root._hasAlphaChannel
	property bool _hasAlphaChannel: false
	signal sourceCopiedToClipboard()

	// The loaded image should not be cached so that the alpha channel information is emitted.
	cache: false
	// The UI should not freeze while the image is generated/loaded.
	asynchronous: true
	fillMode: Image.PreserveAspectCrop
	// The image should not flicker when it is resized.
	retainWhileLoading: true
	// The generated image should fit its item's size.
	sourceSize.width: root.width
	sourceSize.height: root.height
	layer.enabled: GraphicsInfo.api !== GraphicsInfo.Software
	layer.effect: Kirigami.ShadowedTexture {
		radius: root.radius
	}

	Connections {
		target: ImageProvider
		enabled: root.source.toString()

		function onSourceCopiedToClipboard(source: url, size: size) {
			const decoded = decodeURIComponent(source)

			if (decoded.toString() === root.source.toString()) {
				root.sourceCopiedToClipboard()
			}
		}

		function onImageAlphaChannelChanged(source: url, hasAlphaChannel: bool) {
			const decoded = decodeURIComponent(source)

			if (decoded.toString() === root.source.toString()) {
				root._hasAlphaChannel = hasAlphaChannel
			}
		}
	}

	function copyToClipboard() {
		if (root.source.toString()) {
			ImageProvider.copySourceToClipboard(root.source, root.sourceSize)
		}
	}
}
