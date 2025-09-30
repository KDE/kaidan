// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import im.kaidan.kaidan

RoundedImage {
	property ContactTrustMessageUriGenerator uriGenerator

	source: ImageProvider.generatedQrCodeImageUrl(uriGenerator.uri)
}
