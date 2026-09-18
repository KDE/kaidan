// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Bhavy Airi <airiraghav@gmail.com>
// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan

import "../elements"
import "../elements/fields"

/**
 * This page is used for the manual registration.
 *
 * Everything can be manually chosen.
 * In case of no input, random values are used.
 * Only required information must be entered to create an account.
 */
RegistrationPage {
	id: root

	property var nextFocusedItem: null

	title: qsTr("Manual Registration")
	username: usernameField.text.length > 0 ? usernameField.text : usernameField.placeholderText
	password: passwordField.text.length > 0 ? passwordField.text : passwordField.generatedPassword
	onFormUpdated: {
		usernameField.visible = formModel.hasUsernameField()
		passwordField.visible = formModel.hasPasswordField()

		forceActiveFocus()
	}

	ColumnLayout {
		Kirigami.Heading {
			text: qsTr("Complete the registration form!")
			level: 1
			wrapMode: Text.Wrap
			horizontalAlignment: Text.AlignHCenter
			verticalAlignment: Text.AlignVCenter
			Layout.margins: Kirigami.Units.largeSpacing
			Layout.bottomMargin: Kirigami.Units.largeSpacing * 2
			Layout.fillWidth: true
		}

		Image {
			source: Utils.getResourcePath("images/onboarding/custom-form.svg")
			fillMode: Image.PreserveAspectFit
			Layout.alignment: Qt.AlignHCenter
			Layout.bottomMargin: Kirigami.Units.gridUnit
			Layout.fillWidth: true
		}

		FormCard.FormCard {
			visible: root.account.connection.error !== ClientController.EmailConfirmationRequired && (displayNameField.visible || usernameField.visible || passwordField.visible)

			FormCard.FormHeader {
				title: qsTr("Enter your desired credentials")
			}

			Field {
				id: displayNameField
				label: qsTr("Display name")
				background: NonInteractiveFormDelegateBackground {}
				visible: !AccountController.migrating
				inputMethodHints: Qt.ImhPreferUppercase
				onAccepted: usernameField.forceActiveFocus()
			}

			Field {
				id: usernameField
				label: qsTr("Username")
				text: displayNameField.text.replace(/ /g, ".").toLowerCase()
				placeholderText: credentialsGenerator.generateUsername()
				background: NonInteractiveFormDelegateBackground {}
				inputMethodHints: Qt.ImhPreferLowercase
				invalidHintText: qsTr("Enter a valid username or leave the field empty for a random one")
				inputValidator.patterns: InputValidator.Pattern.Username | InputValidator.Pattern.Empty
				onAccepted: passwordField.forceActiveFocus()

				function regenerateUsername() {
					placeholderText = credentialsGenerator.generateUsername()
				}
			}

			RegistrationPasswordField {
				id: passwordField
				background: NonInteractiveFormDelegateBackground {}
				onAccepted: customDataFormCard.visible ? customDataFormCard.forceActiveFocus() : registerWithoutClickingRegistrationButton()
			}
		}

		CustomDataFormCard {
			id: customDataFormCard
			model: root.formFilterModel
			lastTextFieldAcceptedFunction: registerWithoutClickingRegistrationButton
			visible: root.account.connection.error !== ClientController.EmailConfirmationRequired && root.customFormFieldsAvailable
			Layout.fillWidth: true
		}

		RegistrationButton {
			id: registrationButton
			account: root.account
			loginFunction: logIn
			registrationFunction: register
			Layout.fillWidth: true
		}
	}

	Connections {
		target: root.account.registrationController

		function onRegistrationOutOfBandUrlReceived(outOfBandUrl) {
			// Further processing is handled by ProviderPage.
			popLayer()
		}

		/**
		 * Informs the user about registration errors.
		 *
		 * Depending on the error, the field is focused whose input should be corrected.
		 * For all remaining errors, this page is closed.
		 */
		function onRegistrationFailed(error, errorMessage) {
			switch(error) {
			case RegistrationController.InBandRegistrationNotSupported:
			case RegistrationController.TemporarilyBlocked:
				// Further processing is handled by ProviderPage.
				popLayer()
				break
			case RegistrationController.UsernameConflict:
				handleUsernameConflictError()
				break
			case RegistrationController.PasswordTooWeak:
				nextFocusedItem = passwordField
				requestRegistrationForm()
				passiveNotification(qsTr("The provider requires a stronger password."))
				break
			case RegistrationController.CaptchaVerificationFailed:
				nextFocusedItem = customDataFormCard
				requestRegistrationForm()
				showPassiveNotificationForCaptchaVerificationFailedError()
				break
			case RegistrationController.RequiredInformationMissing:
				requestRegistrationForm()
				if (customDataFormCard.visible) {
					nextFocusedItem = customDataFormCard
					showPassiveNotificationForRequiredInformationMissingError(errorMessage)
				} else {
					showPassiveNotificationForUnknownError(errorMessage)
				}
				break
			default:
				showPassiveNotificationForUnknownError(errorMessage)
				popLayer()
			}
		}
	}

	function forceActiveFocus() {
		if (nextFocusedItem) {
			nextFocusedItem.forceActiveFocus()
		} else {
			if (displayNameField.visible) {
				displayNameField.forceActiveFocus()
			} else if (usernameField.visible) {
				usernameField.forceActiveFocus()
			} else if (passwordField.visible) {
				passwordField.forceActiveFocus()
			} else if (customDataFormCard.visible) {
				customDataFormCard.forceActiveFocus()
			}
		}

		nextFocusedItem = null
	}

	/**
	 * Shows a passive notification if a username is already taken on the provider.
	 * If a random username was used for registration, a new one is generated.
	 */
	function handleUsernameConflictError() {
		nextFocusedItem = usernameField
		requestRegistrationForm()

		let notificationText = qsTr("The username is already taken.")

		if (usernameField.text.length === 0) {
			usernameField.regenerateUsername()
			notificationText += " " + qsTr("A new random username has been generated.")
		}

		passiveNotification(notificationText)
	}

	function registerWithoutClickingRegistrationButton() {
		registrationButton.busy = true
		register()
	}

	function register() {
		changeNickname()
		sendRegistrationForm()
	}

	function logIn() {
		changeNickname()
		account.connection.logIn()
	}

	function changeNickname() {
		if (displayNameField.text && !AccountController.migrating) {
			account.settings.name = displayNameField.text
		}
	}
}
