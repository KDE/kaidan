// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

UserKeyAuthenticationButton {
	text: {
		switch(authenticationState) {
		case DetailsContent.EncryptionKeyAuthenticationState.DistrustedKeysOnly:
			return qsTr("Verify your contact's devices to encrypt for them")
		case DetailsContent.EncryptionKeyAuthenticationState.NoKeys:
			return qsTr("Verify your contact's unknown devices to encrypt for them")
		case DetailsContent.EncryptionKeyAuthenticationState.AuthenticatableDistrustedKeys:
			return qsTr("Verify your contact's devices to encrypt for them")
		case DetailsContent.EncryptionKeyAuthenticationState.AuthenticatableTrustedKeysOnly:
			return qsTr("Verify your contact's devices for maximum security")
		case DetailsContent.EncryptionKeyAuthenticationState.AllKeysAuthenticated:
			return qsTr("Verify your contact's unknown devices or let your contact verify this device")
		}
	}
	description: {
		switch(authenticationState) {
		case DetailsContent.EncryptionKeyAuthenticationState.DistrustedKeysOnly:
			return qsTr("Your contact has no trusted device to encrypt for")
		case DetailsContent.EncryptionKeyAuthenticationState.NoKeys:
			return qsTr("Your contact has no known device using OMEMO 2")
		case DetailsContent.EncryptionKeyAuthenticationState.AuthenticatableDistrustedKeys:
			return qsTr("Your contact has devices that need to be verified to encrypt for them")
		case DetailsContent.EncryptionKeyAuthenticationState.AuthenticatableTrustedKeysOnly:
			return qsTr("Your contact has devices that are not verified")
		case DetailsContent.EncryptionKeyAuthenticationState.AllKeysAuthenticated:
			return qsTr("All your contact's known devices are verified")
		}
	}
	icon.name: {
		switch(authenticationState) {
		case DetailsContent.EncryptionKeyAuthenticationState.DistrustedKeysOnly:
			return "channel-secure-symbolic"
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
}
