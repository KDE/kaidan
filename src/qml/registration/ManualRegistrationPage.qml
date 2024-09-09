// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Bhavy Airi <airiraghav@gmail.com>
// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigami 2.19 as Kirigami
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

import im.kaidan.kaidan 1.0

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

		MobileForm.FormCard {
			visible: Kaidan.connectionError !== ClientWorker.EmailConfirmationRequired && (displayNameField.visible || usernameField.visible || passwordField.visible)
			Layout.fillWidth: true
			contentItem: ColumnLayout {
				spacing: 0

				MobileForm.FormCardHeader {
					title: qsTr("Enter your desired credentials")
				}

				MobileForm.FormCard {
					Layout.fillWidth: true
					Kirigami.Theme.colorSet: Kirigami.Theme.Window
					contentItem: MobileForm.AbstractFormDelegate {
						background: Item {}
						contentItem: ColumnLayout {
							Field {
								id: displayNameField
								labelText: qsTr("Display Name")
								visible: Kaidan.testAccountMigrationState(AccountMigrationManager.MigrationState.Idle)
								inputMethodHints: Qt.ImhPreferUppercase
								inputField.onAccepted: usernameField.forceActiveFocus()
							}

							CredentialsField {
								id: usernameField
								labelText: qsTr("Username")
								text: displayNameField.text.replace(/ /g, ".").toLowerCase()
								placeholderText: credentialsGenerator.generateUsername()
								inputMethodHints: Qt.ImhPreferLowercase
								invalidHintText: qsTr("Enter a valid username or leave the field empty for a random one")
								valid: true
								onTextChanged: {
									if (text === "") {
										valid = true
									} else {
										valid = credentialsValidator.isUsernameValid(text)
									}

									toggleHintForInvalidText()
								}
								inputField.onAccepted: passwordField.forceActiveFocus()

								function regenerateUsername() {
									placeholderText = credentialsGenerator.generateUsername()
								}
							}

							RegistrationPasswordField {
								id: passwordField
								onTextChanged: {
									if (text === "") {
										valid = true
									} else {
										valid = credentialsValidator.isPasswordValid(text)
									}

									toggleHintForInvalidText()
								}
								inputField.onAccepted: customDataFormArea.forceActiveFocus()
							}
						}
					}
				}
			}
		}

		CustomDataFormArea {
			id: customDataFormArea
			model: root.formFilterModel
			lastTextFieldAcceptedFunction: registerWithoutClickingRegistrationButton
			visible: Kaidan.connectionError !== ClientWorker.EmailConfirmationRequired && root.customFormFieldsAvailable
			Layout.fillWidth: true
		}

		RegistrationButton {
			id: registrationButton
			loginFunction: logIn
			registrationFunction: register
			Layout.fillWidth: true
		}
	}

	Connections {
		target: Kaidan

		function onRegistrationFormReceived(dataFormModel) {
			processFormModel(dataFormModel)
		}

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
			case RegistrationManager.InBandRegistrationNotSupported:
			case RegistrationManager.TemporarilyBlocked:
				// Further processing is handled by ProviderPage.
				popLayer()
				break
			case RegistrationManager.UsernameConflict:
				handleUsernameConflictError()
				break
			case RegistrationManager.PasswordTooWeak:
				nextFocusedItem = passwordField
				requestRegistrationForm()
				passiveNotification(qsTr("The provider requires a stronger password."))
				break
			case RegistrationManager.CaptchaVerificationFailed:
				nextFocusedItem = customDataFormArea
				requestRegistrationForm()
				showPassiveNotificationForCaptchaVerificationFailedError()
				break
			case RegistrationManager.RequiredInformationMissing:
				requestRegistrationForm()
				if (customDataFormArea.visible) {
					nextFocusedItem = customDataFormArea
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

	function processFormModel(dataFormModel) {
		formModel = dataFormModel

		usernameField.visible = dataFormModel.hasUsernameField()
		passwordField.visible = dataFormModel.hasPasswordField()

		forceActiveFocus()
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
			} else if (customDataFormArea.visible) {
				customDataFormArea.forceActiveFocus()
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
		Kaidan.logIn()
	}

	function changeNickname() {
		if (Kaidan.testAccountMigrationState(AccountMigrationManager.MigrationState.Idle)) {
			Kaidan.client.vCardManager.changeNicknameRequested(displayNameField.text)
		}
	}
}
