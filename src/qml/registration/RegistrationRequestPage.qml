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
	property Account account

	onBackRequested: account.registrationController.abortRegistration()

	/**
	 * Requests a registration form from the provider.
	 */
	function requestRegistrationForm() {
		// Request a registration form.
		account.registrationController.requestRegistrationForm()
	}
}
