// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick

import im.kaidan.kaidan

/**
 * This is the base for pages to register an account.
 */
RegistrationRequestPage {
	id: root

	// This model contains all fields from the registration form of the requested provider.
	property DataFormModel formModel

	// This model only contains the custom fields from the registration form of the requested
	// provider.
	// E.g., it may contain a CAPTCHA or an email address.
	// The provider may not use the standard way for requesting the username and the password.
	// In that case, this model could also include fields for those values.
	readonly property RegistrationDataFormFilterModel formFilterModel: RegistrationDataFormFilterModel {
		sourceModel: formModel
	}

	/**
	 * Returns true if the registration form received from the provider contains custom fields.
	 */
	readonly property bool customFormFieldsAvailable: !formFilterModel.isEmpty

	// generator for random usernames and passwords
	readonly property CredentialsGenerator credentialsGenerator: CredentialsGenerator {}

	// username of the user to be registered
	property string username

	// password of the user to be registered
	property string password

	// Emitted if a new registration form (including custom fields) is ready-to-use.
	signal formUpdated

	Connections {
		target: Kaidan.registrationController

		function onRegistrationFormReceived(dataFormModel) {
			root.formModel = dataFormModel
			root.formUpdated()
		}
	}

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
	 * Sends the completed registration form to the provider.
	 */
	function sendRegistrationForm() {
		if (formModel.hasUsernameField()) {
			formModel.setUsername(username)
		} if (formModel.hasPasswordField()) {
			formModel.setPassword(password)
		}

		Kaidan.registrationController.sendRegistrationForm()
	}
}
