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
	title: qsTr("Register automatically")

	property int maximumRegistrationAttemptsAfterUsernameConflictOccurred: 2
	property int remainingRegistrationAttemptsAfterUsernameConflictOccurred: maximumRegistrationAttemptsAfterUsernameConflictOccurred

	property int maximumRegistrationAttemptsAfterCaptchaVerificationFailedOccurred: 1
	property int remainingRegistrationAttemptsAfterCaptchaVerificationFailedOccurred: maximumRegistrationAttemptsAfterCaptchaVerificationFailedOccurred

	property int maximumRegistrationAttemptsAfterRequiredInformationMissingOccurred: 2
	property int remainingRegistrationAttemptsAfterRequiredInformationMissingOccurred: maximumRegistrationAttemptsAfterRequiredInformationMissingOccurred

	property var triedProviderIndexes: []

	ProviderListModel {
		id: providerListModel
	}

	ColumnLayout {
		anchors.fill: parent

		Controls.StackView {
			id: stackView
			clip: true
			Layout.fillWidth: true
			Layout.fillHeight: true
			initialItem: loadingViewComponent
		}
	}

	Component { id: loadingViewComponent; LoadingView {} }

	Component { id: customFormViewComponent; CustomFormViewAutomaticRegistration {}	}

	Component.onCompleted: {
		chooseProviderRandomly()
		chooseUsernameRandomly()
		choosePasswordRandomly()
		requestRegistrationForm()
	}

	Connections {
		target: Kaidan

		function onRegistrationFormReceived(dataFormModel) {
			formModel = dataFormModel
			formFilterModel.sourceModel = dataFormModel

			// If the received registration data form does not contain custom fields, request the registration directly.
			// Otherwise, remove the loading view to show the custom form view.
			if (!customFormFieldsAvailable())
				sendRegistrationForm()
			else
				removeLoadingView()
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

				if (remainingRegistrationAttemptsAfterRequiredInformationMissingOccurred > 0 && customFormFieldsAvailable()) {
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
					removeLoadingView()
				} else {
					popLayer()
				}
			}
		}
	}

	// Simulate the pressing of the confirmation button.
	Keys.onPressed: {
		if (stackView.currentItem.registrationButton && (event.key === Qt.Key_Return || event.key === Qt.Key_Enter))
			stackView.currentItem.registrationButton.clicked()
	}

	/**
	 * Adds the loading view to the stack view.
	 */
	function addLoadingView() {
		stackView.push(loadingViewComponent)
	}

	/**
	 * Removes the loading view from the stack view.
	 */
	function removeLoadingView() {
		// Push the custom form view on the stack view if the loading view is the only item on it.
		// That is the case during the first registration form request.
		// When the loading view was pushed on the stack view after submitting custom form fields and the registration fails or a connection error occurs, that loading view is popped to show the custom form view again.
		if (stackView.depth === 1) {
			stackView.push(customFormViewComponent)
			stackView.currentItem.registrationButton.parent = stackView.currentItem.contentArea
		} else {
			stackView.pop()
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

	/**
	 * Sends the completed registration form and adds the loading view.
	 */
	function sendRegistrationFormAndShowLoadingView() {
		sendRegistrationForm()
		addLoadingView()
	}
}
