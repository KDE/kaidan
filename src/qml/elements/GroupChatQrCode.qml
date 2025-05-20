// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import im.kaidan.kaidan

RoundedImage {
	property alias jid: qrCodeGenerator.jid

	source: qrCodeGenerator.qrCode

	GroupChatQrCodeGenerator {
		id: qrCodeGenerator
		edgePixelCount: parent.width
	}
}
