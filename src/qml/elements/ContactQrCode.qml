// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import im.kaidan.kaidan 1.0

QrCode {
	id: root

	property string accountJid

	source: qrCodeGenerator.qrCode

	ContactQrCodeGenerator {
		id: qrCodeGenerator
		accountJid: root.accountJid
		jid: root.jid
		edgePixelCount: root.width
	}
}
