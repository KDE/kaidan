// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import im.kaidan.kaidan

RoundedImage {
	id: root

	property alias accountSettings: qrCodeGenerator.accountSettings

	source: qrCodeGenerator.qrCode

	LoginQrCodeGenerator {
		id: qrCodeGenerator
		edgePixelCount: root.width
	}
}
