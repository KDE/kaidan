// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

UserKeyAuthenticationButton {
	text: {
		switch(authenticationState) {
		case DetailsContent.EncryptionKeyAuthenticationState.DistrustedKeysOnly:
			return qsTr("Verify the participants' devices to encrypt for them")
		case DetailsContent.EncryptionKeyAuthenticationState.NoKeys:
			return qsTr("Verify the participants' unknown devices to encrypt for them")
		case DetailsContent.EncryptionKeyAuthenticationState.AuthenticatableDistrustedKeys:
			return qsTr("Verify the participants' devices to encrypt for them")
		case DetailsContent.EncryptionKeyAuthenticationState.AuthenticatableTrustedKeysOnly:
			return qsTr("Verify the participants' devices for maximum security")
		case DetailsContent.EncryptionKeyAuthenticationState.AllKeysAuthenticated:
			return qsTr("Verify the participants' unknown devices or let the participants verify this device")
		}
	}
	description: {
		switch(authenticationState) {
		case DetailsContent.EncryptionKeyAuthenticationState.DistrustedKeysOnly:
			return qsTr("No participant has a trusted device to encrypt for")
		case DetailsContent.EncryptionKeyAuthenticationState.NoKeys:
			return qsTr("No participant has a known device using OMEMO 2")
		case DetailsContent.EncryptionKeyAuthenticationState.AuthenticatableDistrustedKeys:
			return qsTr("Some participants have devices that need to be verified to encrypt for them")
		case DetailsContent.EncryptionKeyAuthenticationState.AuthenticatableTrustedKeysOnly:
			return qsTr("Some participants have devices that are not verified")
		case DetailsContent.EncryptionKeyAuthenticationState.AllKeysAuthenticated:
			return qsTr("All participants' devices are verified")
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
