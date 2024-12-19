// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan

FormCard.FormButtonDelegate {
	property EncryptionWatcher encryptionWatcher
	property int authenticationState
}
