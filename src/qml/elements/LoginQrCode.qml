// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import im.kaidan.kaidan

RoundedImage {
	property AccountSettings accountSettings

	source: ImageProvider.generatedQrCodeImageUrl(accountSettings.loginUriString)
}
