// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import im.kaidan.kaidan 1.0

KeyAuthenticationButton {
	property alias jid: resourcesWatcher.jid

	authenticationState: {
		if (!encryptionWatcher.hasUsableDevices) {
			if (encryptionWatcher.hasDistrustedDevices) {
				return DetailsContent.EncryptionKeyAuthenticationState.DistrustedKeysOnly
			}

			if (resourcesWatcher.resourcesCount > 1) {
				return DetailsContent.EncryptionKeyAuthenticationState.ResourcesWithoutKeys
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
	text: {
		switch(authenticationState) {
		case DetailsContent.EncryptionKeyAuthenticationState.DistrustedKeysOnly:
			return qsTr("Verify your own devices to encrypt for them")
		case DetailsContent.EncryptionKeyAuthenticationState.ResourcesWithoutKeys:
		case DetailsContent.EncryptionKeyAuthenticationState.NoKeys:
			return qsTr("Verify own unknown devices to encrypt for them")
		case DetailsContent.EncryptionKeyAuthenticationState.AuthenticatableDistrustedKeys:
			return qsTr("Verify your own devices to encrypt for them")
		case DetailsContent.EncryptionKeyAuthenticationState.AuthenticatableTrustedKeysOnly:
			return qsTr("Verify your own devices for maximum security")
		case DetailsContent.EncryptionKeyAuthenticationState.AllKeysAuthenticated:
			return qsTr("Verify own unknown devices or let your other devices verify this device")
		}
	}
	description: {
		switch(authenticationState) {
		case DetailsContent.EncryptionKeyAuthenticationState.DistrustedKeysOnly:
			return qsTr("You have no other trusted device to encrypt for")
		case DetailsContent.EncryptionKeyAuthenticationState.ResourcesWithoutKeys:
			return qsTr("You have other devices that do not use OMEMO 2")
		case DetailsContent.EncryptionKeyAuthenticationState.NoKeys:
			return qsTr("You have no known other device using OMEMO 2")
		case DetailsContent.EncryptionKeyAuthenticationState.AuthenticatableDistrustedKeys:
			return qsTr("You have other devices that need to be verified to encrypt for them")
		case DetailsContent.EncryptionKeyAuthenticationState.AuthenticatableTrustedKeysOnly:
			return qsTr("You have other devices that are not verified")
		case DetailsContent.EncryptionKeyAuthenticationState.AllKeysAuthenticated:
			return qsTr("All your other known devices are verified")
		}
	}
	icon.name: {
		switch(authenticationState) {
		case DetailsContent.EncryptionKeyAuthenticationState.DistrustedKeysOnly:
			return "channel-secure-symbolic"
		case DetailsContent.EncryptionKeyAuthenticationState.ResourcesWithoutKeys:
		case DetailsContent.EncryptionKeyAuthenticationState.NoKeys:
			return "channel-insecure-symbolic"
		case DetailsContent.EncryptionKeyAuthenticationState.AuthenticatableDistrustedKeys:
			return "security-medium-symbolic"
		case DetailsContent.EncryptionKeyAuthenticationState.AuthenticatableTrustedKeysOnly:
			return "security-high-symbolic"
		case DetailsContent.EncryptionKeyAuthenticationState.AllKeysAuthenticated:
			return "security-medium-symbolic"
		}
	}

	UserResourcesWatcher {
		id: resourcesWatcher
	}
}
