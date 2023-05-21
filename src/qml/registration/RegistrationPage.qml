// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

/**
 * This is the base of a registration page.
 */
Kirigami.Page {
	// This model contains all fields from the registration form of the requested provider.
	property DataFormModel formModel

	// This model only contains the custom fields from the registration form of the requested provider.
	// It may contain e.g. a CAPTCHA or an email address.
	// The provider may not use the standard way for requesting the username and the password.
	// In that case, this model could also include fields for those values.
	property alias formFilterModel: formFilterModel

	// generator for random usernames and passwords
	property alias credentialsGenerator: credentialsGenerator

	// JID of the provider from whom the registration form is requested
	property string provider

	// username of the user to be registered
	property string username

	// password of the user to be registered
	property string password

	leftPadding: 0
	topPadding: 0
	rightPadding: 0
	bottomPadding: 0

	RegistrationDataFormFilterModel {
		id: formFilterModel
	}

	CredentialsGenerator {
		id: credentialsGenerator
	}

	Component.onCompleted: AccountManager.resetCustomConnectionSettings()

	/**
	 * Shows a passive notification if the CAPTCHA verification failed.
	 */
	function showPassiveNotificationForCaptchaVerificationFailedError() {
		passiveNotification(qsTr("CAPTCHA input was wrong"))
	}

	/**
	 * Shows a passive notification if required information is missing.
	 *
	 * @param errorMessage text describing the error and the required missing information
	 */
	function showPassiveNotificationForRequiredInformationMissingError(errorMessage) {
		passiveNotification(qsTr("Required information is missing: ") + errorMessage)
	}

	/**
	 * Shows a passive notification for an unknown error.
	 *
	 * @param errorMessage text describing the error
	 */
	function showPassiveNotificationForUnknownError(errorMessage) {
		passiveNotification(qsTr("Registration failed:") + " " + errorMessage)
	}

	/**
	 * Returns true if the registration form received from the provider contains custom fields.
	 */
	function customFormFieldsAvailable() {
		return formFilterModel.rowCount() > 0
	}

	/**
	 * Requests a registration form from the provider.
	 */
	function requestRegistrationForm() {
		// Set the provider's JID.
		AccountManager.jid = provider

		// Request a registration form.
		Kaidan.requestRegistrationForm()
	}

	/**
	 * Sends the completed registration form to the provider.
	 */
	function sendRegistrationForm() {
		if (formModel.hasUsernameField())
			formModel.setUsername(username)
		if (formModel.hasPasswordField())
			formModel.setPassword(password)

		Kaidan.client.registrationManager.sendRegistrationFormRequested()
	}
}
