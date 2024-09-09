// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

/**
 * This is the base of a registration page.
 */
Kirigami.ScrollablePage {
	// This model contains all fields from the registration form of the requested provider.
	property DataFormModel formModel

	// This model only contains the custom fields from the registration form of the requested
	// provider.
	// E.g., it may contain a CAPTCHA or an email address.
	// The provider may not use the standard way for requesting the username and the password.
	// In that case, this model could also include fields for those values.
	property alias formFilterModel: formFilterModel

	/**
	 * Returns true if the registration form received from the provider contains custom fields.
	 */
	property bool customFormFieldsAvailable: !formFilterModel.isEmpty

	// generator for random usernames and passwords
	property alias credentialsGenerator: credentialsGenerator

	// JID of the provider from whom the registration form is requested
	property string provider

	// username of the user to be registered
	property string username

	// password of the user to be registered
	property string password

	horizontalPadding: 0
	background: Rectangle {
		color: secondaryBackgroundColor

		Image {
			source: Utils.getResourcePath("images/chat-page-background.svg")
			anchors.fill: parent
			fillMode: Image.Tile
			horizontalAlignment: Image.AlignLeft
			verticalAlignment: Image.AlignTop
		}
	}
	onBackRequested: Kaidan.client.registrationManager.abortRegistrationRequested()
	Component.onCompleted: AccountManager.resetCustomConnectionSettings()

	RegistrationDataFormFilterModel {
		id: formFilterModel
		sourceModel: formModel
	}

	CredentialsGenerator {
		id: credentialsGenerator
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
	 * Requests a registration form from the provider.
	 */
	function requestRegistrationForm() {
		// Set the provider's JID.
		AccountManager.jid = provider

		// Request a registration form.
		Kaidan.client.registrationManager.registrationFormRequested()
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

		Kaidan.client.registrationManager.sendRegistrationFormRequested()
	}
}
