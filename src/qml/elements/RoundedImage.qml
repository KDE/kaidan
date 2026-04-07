// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2025 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Effects
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

/**
 * Image with rounded corners.
 */
Image {
	id: root

	property alias radius: roundedCornerMask.radius
	property bool blurEnabled: false
	property int blurMax: 64
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
	sourceSize.width: width
	sourceSize.height: height
	layer {
		enabled: GraphicsInfo.api !== GraphicsInfo.Software && !_hasAlphaChannel
		effect: MultiEffect {
			maskEnabled: true
			maskSource: roundedCornerMask
			maskThresholdMin: 0.5
			maskSpreadAtMin: 1
			blurEnabled: root.blurEnabled
			blurMax: root.blurMax
			blur: 1
			autoPaddingEnabled: false
		}
	}

	Rectangle {
		id: roundedCornerMask
		width: parent.width
		height: parent.height
		layer.enabled: true
		visible: false
		radius: relativeRoundedCornersRadius(width, height)
		color: "black"
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
		if (source.toString()) {
			ImageProvider.copySourceToClipboard(source, sourceSize)
		}
	}
}
