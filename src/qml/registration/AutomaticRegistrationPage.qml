// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan
import "../elements"

/**
 * This page is used for the automatic registration.
 *
 * Everything that can be automated is automated.
 * The provider is randomly selected.
 * The username and password are randomly generated.
 * Therefore, only custom information requested by the provider is shown.
 * Only required information must be entered to register an account.
 */
RegistrationPage {
	id: root
	title: qsTr("Automatic Registration")

	property int maximumRegistrationAttemptsAfterUsernameConflictOccurred: 2
	property int remainingRegistrationAttemptsAfterUsernameConflictOccurred: maximumRegistrationAttemptsAfterUsernameConflictOccurred

	property int maximumRegistrationAttemptsAfterCaptchaVerificationFailedOccurred: 1
	property int remainingRegistrationAttemptsAfterCaptchaVerificationFailedOccurred: maximumRegistrationAttemptsAfterCaptchaVerificationFailedOccurred

	property int maximumRegistrationAttemptsAfterRequiredInformationMissingOccurred: 2
	property int remainingRegistrationAttemptsAfterRequiredInformationMissingOccurred: maximumRegistrationAttemptsAfterRequiredInformationMissingOccurred

	property var triedProviderIndexes: []

	onFormUpdated: {
		if (customFormFieldsAvailable) {
			loadingStackArea.busy = false
		} else {
			sendRegistrationForm()
		}
	}
	Component.onCompleted: {
		chooseProviderRandomly()
		chooseUsernameRandomly()
		choosePasswordRandomly()
		requestRegistrationForm()
	}

	LoadingStackArea {
		id: loadingStackArea
		busy: true
		loadingArea.description: qsTr("Requesting registration…")
		loadingArea.background.color: primaryBackgroundColor

		ColumnLayout {
			Kirigami.Heading {
				text: qsTr("Complete the registration!")
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

			CustomDataFormArea {
				model: root.formFilterModel
				lastTextFieldAcceptedFunction: registerWithoutClickingRegistrationButton
				visible: root.account.connection.error !== ClientController.EmailConfirmationRequired
				Layout.fillWidth: true
				onVisibleChanged: {
					if (visible) {
						forceActiveFocus()
					}
				}
			}

			RegistrationButton {
				id: registrationButton
				account: root.account
				registrationFunction: register
				loginFunction: logIn
				Layout.fillWidth: true
			}
		}
	}

	ProviderModel {
		id: providerModel
	}

	Connections {
		target: root.account.connection

		function onErrorChanged() {
			if (root.account.connection.error !== ClientController.NoError) {
				if (root.account.connection.error === ClientController.EmailConfirmationRequired) {
					loadingStackArea.busy = false
				} else if (root.triedProviderIndexes.length < providerModel.rowCount()){
					requestRegistrationFormFromAnotherProvider()
				} else {
					passiveNotification(qsTr("Automatic registration failed because of problems with all providers"))
					popLayer()
				}
			}
		}
	}

	Connections {
		target: root.account.registrationController

		function onRegistrationOutOfBandUrlReceived(outOfBandUrl) {
			requestRegistrationFormFromAnotherProvider()
		}

		function onRegistrationFailed(error, errorMessage) {
			switch (error) {
			case RegistrationController.UsernameConflict:
				if (remainingRegistrationAttemptsAfterUsernameConflictOccurred > 0) {
					remainingRegistrationAttemptsAfterUsernameConflictOccurred--
					// Try to register again with another username on the same provider.
					chooseUsernameRandomly()
					requestRegistrationForm()
				} else {
					remainingRegistrationAttemptsAfterUsernameConflictOccurred = maximumRegistrationAttemptsAfterUsernameConflictOccurred
					requestRegistrationFormFromAnotherProvider()
				}
				break
			case RegistrationController.CaptchaVerificationFailed:
				showPassiveNotificationForCaptchaVerificationFailedError(errorMessage)

				if (remainingRegistrationAttemptsAfterCaptchaVerificationFailedOccurred > 0) {
					remainingRegistrationAttemptsAfterCaptchaVerificationFailedOccurred--
					requestRegistrationForm()
				} else {
					remainingRegistrationAttemptsAfterCaptchaVerificationFailedOccurred = maximumRegistrationAttemptsAfterCaptchaVerificationFailedOccurred
					requestRegistrationFormFromAnotherProvider()
				}
				break
			case RegistrationController.RequiredInformationMissing:
				showPassiveNotificationForRequiredInformationMissingError(errorMessage)

				if (remainingRegistrationAttemptsAfterRequiredInformationMissingOccurred > 0 && customFormFieldsAvailable) {
					remainingRegistrationAttemptsAfterRequiredInformationMissingOccurred--
					requestRegistrationForm()

				} else {
					remainingRegistrationAttemptsAfterRequiredInformationMissingOccurred = remainingRegistrationAttemptsAfterRequiredInformationMissingOccurred
					requestRegistrationFormFromAnotherProvider()
				}
				break
			default:
				requestRegistrationFormFromAnotherProvider()
			}
		}
	}

	/**
	 * Sets a randomly chosen provider for registration while already tried providers are excluded.
	 *
	 * @param providersMatchingSystemLocaleOnly whether to solely choose providers that match the
	 *        system's locale
	 */
	function chooseProviderRandomly(providersMatchingSystemLocaleOnly = true) {
		const chosenIndex = providerModel.randomlyChooseIndex(triedProviderIndexes, providersMatchingSystemLocaleOnly)
		triedProviderIndexes.push(chosenIndex)
		root.account.settings.jid = providerModel.data(chosenIndex, ProviderModel.Jid)
	}

	/**
	 * Sets a randomly generated username for registration.
	 */
	function chooseUsernameRandomly() {
		username = credentialsGenerator.generateUsername()
	}

	/**
	 * Sets a randomly generated password for registration.
	 */
	function choosePasswordRandomly() {
		password = credentialsGenerator.generatePassword()
	}

	/**
	 * Requests a registration form from another randomly chosen provider.
	 */
	function requestRegistrationFormFromAnotherProvider() {
		if (triedProviderIndexes.length < providerModel.providersMatchingSystemLocaleMinimumCount) {
			chooseProviderRandomly()
		} else {
			chooseProviderRandomly(false)
		}

		requestRegistrationForm()
	}

	function registerWithoutClickingRegistrationButton() {
		registrationButton.busy = true
		register()
	}

	/**
	 * Sends the completed registration form.
	 */
	function register() {
		sendRegistrationForm()
		loadingStackArea.busy = true
	}

	/**
	 * Logs in after a registration that needs to be confirmed.
	 */
	function logIn() {
		loadingStackArea.busy = true
		account.connection.logIn()
	}
}
