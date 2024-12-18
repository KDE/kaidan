// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import im.kaidan.kaidan

QrCode {
	id: root
	source: qrCodeGenerator.qrCode

	GroupChatQrCodeGenerator {
		id: qrCodeGenerator
		jid: root.jid
		edgePixelCount: root.width
	}
}
