// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick

import im.kaidan.kaidan

import ".."

/**
 * This is the base for pages requesting to register an account.
 */
ImageBackgroundPage {
	// JID of the provider from whom the registration form is requested
	property string provider

	onBackRequested: Kaidan.registrationController.abortRegistration()
	Component.onCompleted: AccountController.resetCustomConnectionSettings()

	/**
	 * Requests a registration form from the provider.
	 */
	function requestRegistrationForm() {
		// Set the provider's JID.
		AccountController.setNewAccountJid(provider)

		// Request a registration form.
		Kaidan.registrationController.requestRegistrationForm()
	}
}
