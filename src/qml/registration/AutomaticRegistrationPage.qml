// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0
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
				id: customDataFormArea
				model: root.formFilterModel
				lastTextFieldAcceptedFunction: registerWithoutClickingRegistrationButton
				visible: Kaidan.connectionError !== ClientWorker.EmailConfirmationRequired
				Layout.fillWidth: true
				onVisibleChanged: {
					if (visible) {
						forceActiveFocus()
					}
				}
			}

			RegistrationButton {
				id: registrationButton
				registrationFunction: register
				loginFunction: logIn
				Layout.fillWidth: true
			}
		}
	}

	ProviderListModel {
		id: providerListModel
	}

	Connections {
		target: Kaidan

		function onRegistrationFormReceived(dataFormModel) {
			formModel = dataFormModel

			if (customFormFieldsAvailable) {
				loadingStackArea.busy = false
			} else {
				sendRegistrationForm()
			}
		}

		function onRegistrationOutOfBandUrlReceived(outOfBandUrl) {
			requestRegistrationFormFromAnotherProvider()
		}

		function onRegistrationFailed(error, errorMessage) {
			switch (error) {
			case RegistrationManager.UsernameConflict:
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
			case RegistrationManager.CaptchaVerificationFailed:
				showPassiveNotificationForCaptchaVerificationFailedError(errorMessage)

				if (remainingRegistrationAttemptsAfterCaptchaVerificationFailedOccurred > 0) {
					remainingRegistrationAttemptsAfterCaptchaVerificationFailedOccurred--
					requestRegistrationForm()
				} else {
					remainingRegistrationAttemptsAfterCaptchaVerificationFailedOccurred = maximumRegistrationAttemptsAfterCaptchaVerificationFailedOccurred
					requestRegistrationFormFromAnotherProvider()
				}
				break
			case RegistrationManager.RequiredInformationMissing:
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

		function onConnectionErrorChanged() {
			if (Kaidan.connectionError !== ClientWorker.NoError) {
				if (Kaidan.connectionError === ClientWorker.EmailConfirmationRequired) {
					loadingStackArea.busy = false
				} else {
					popLayer()
				}
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
		const chosenIndex = providerListModel.randomlyChooseIndex(triedProviderIndexes, providersMatchingSystemLocaleOnly)
		triedProviderIndexes.push(chosenIndex)
		provider = providerListModel.data(chosenIndex, ProviderListModel.JidRole)
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
		if (triedProviderIndexes.length < providerListModel.providersMatchingSystemLocaleMinimumCount) {
			chooseProviderRandomly()
			requestRegistrationForm()
		} else {
			chooseProviderRandomly(false)
			requestRegistrationForm()
		}
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
		Kaidan.logIn()
	}
}
