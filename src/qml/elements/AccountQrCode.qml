// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import im.kaidan.kaidan

RoundedImage {
	property alias uriGenerator: qrCodeGenerator.uriGenerator

	source: qrCodeGenerator.qrCode

	AccountQrCodeGenerator {
		id: qrCodeGenerator
		edgePixelCount: parent.width
	}
}
