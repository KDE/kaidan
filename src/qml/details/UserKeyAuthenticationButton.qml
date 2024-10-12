// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

KeyAuthenticationButton {
	authenticationState: {
		if (!encryptionWatcher.hasUsableDevices) {
			if (encryptionWatcher.hasDistrustedDevices) {
				return DetailsContent.EncryptionKeyAuthenticationState.DistrustedKeysOnly
			}

			return DetailsContent.EncryptionKeyAuthenticationState.NoKeys
		} else if (encryptionWatcher.hasAuthenticatableDevices) {
			if (encryptionWatcher.hasAuthenticatableDistrustedDevices) {
				return DetailsContent.EncryptionKeyAuthenticationState.AuthenticatableDistrustedKeys
			}

			return DetailsContent.EncryptionKeyAuthenticationState.AuthenticatableTrustedKeysOnly
		}

		return DetailsContent.EncryptionKeyAuthenticationState.AllKeysAuthenticated
	}
}
