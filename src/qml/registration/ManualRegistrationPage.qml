// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Bhavy Airi <airiraghav@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls

import im.kaidan.kaidan 1.0

/**
 * This page is used for the manual registration.
 *
 * Everything can be manually chosen.
 * In case of no input, random values are used.
 * Only required information must be entered to create an account.
 */
RegistrationPage {
	title: qsTr("Register manually")

	// These views are created and inserted into the swipe view dynamically.
	// When they are not required, they are removed.
	// The default ones are added to the SwipeView via Component.onCompleted.
	property WebRegistrationView webRegistrationView
	property LoadingView loadingView
	property UsernameView usernameView
	property PasswordView passwordView
	property CustomFormViewManualRegistration customFormView
	property ResultView resultView

	property bool loadingViewActive: loadingView ? loadingView.Controls.SwipeView.isCurrentItem : false
	property bool jumpingToViewsEnabled: !(loadingViewActive || ((usernameView && usernameView.Controls.SwipeView.isCurrentItem || passwordView && passwordView.Controls.SwipeView.isCurrentItem) && !swipeView.currentItem.valid))
	property bool registrationErrorOccurred: false
	property bool connectionErrorOccurred: false

	property alias displayName: displayNameView.text

	provider: providerView.text
	username: usernameView ? usernameView.text : ""
	password: passwordView ? passwordView.text : ""

	ColumnLayout {
		anchors.fill: parent
		spacing: 0

		Controls.SwipeView {
			id: swipeView
			interactive: !loadingViewActive
			Layout.fillWidth: true
			Layout.fillHeight: true
			property int lastIndex: 0

			onCurrentIndexChanged: {
				if (connectionErrorOccurred) {
					connectionErrorOccurred = false
				} else if (!loadingViewActive && currentIndex > lastIndex && currentIndex === providerView.Controls.SwipeView.index + 1 && !webRegistrationView) {
					addLoadingView(currentIndex)
					requestRegistrationForm()
				}

				lastIndex = currentIndex
			}

			DisplayNameView {
				id: displayNameView
			}

			ProviderView {
				id: providerView
			}

			// All dynamically loaded views are inserted here when needed.
		}

		NavigationBar {
			id: navigationBar
		}
	}

	Component {id: webRegistrationViewComponent; WebRegistrationView {}}
	Component {id: loadingViewComponent; LoadingView {}}
	Component {id: usernameViewComponent; UsernameView {}}
	Component {id: passwordViewComponent; PasswordView {}}
	Component {id: customFormViewComponent; CustomFormViewManualRegistration {}}
	Component {id: resultViewComponent; ResultView {}}

	Component.onCompleted: addDynamicallyLoadedInBandRegistrationViews()

	Connections {
		target: Kaidan

		function onConnectionErrorChanged() {
			connectionErrorOccurred = true
			jumpToPreviousView()
			removeLoadingView()
		}

		function onRegistrationFormReceived(dataFormModel) {
			formModel = dataFormModel
			formFilterModel.sourceModel = dataFormModel

			let indexToInsert = loadingView.Controls.SwipeView.index

			// There are three cases here:
			//
			// 1. The provider did not include a "username" field.
			// The username view needs to be removed.
			if (!formModel.hasUsernameField()) {
				swipeView.removeItem(usernameView)
			// 2. The provider did include a "username" field, but the provider selected before did not include it and the username view has been removed.
			// The view needs to be added again.
			} else if (!usernameView) {
				addUsernameView(++indexToInsert)
			// 3. The provider did include a "username" field and the username view is already loaded.
			} else {
				indexToInsert++
			}

			// Same logic as for the username view. See above.
			if (!formModel.hasPasswordField()) {
				swipeView.removeItem(passwordView)
			} else if (!passwordView) {
				addPasswordView(++indexToInsert)
			} else {
				indexToInsert++
			}

			// Same logic as for the username view. See above.
			if (!customFormFieldsAvailable()) {
				swipeView.removeItem(customFormView)
			} else if (!customFormView) {
				addCustomFormView(++indexToInsert)
			} else {
				indexToInsert++
			}

			// Only jump to the next view if the registration form was not loaded because a registration error occurred.
			// Depending on the error, the swipe view jumps to a particular view (see onRegistrationFailed).
			if (registrationErrorOccurred)
				registrationErrorOccurred = false
			else if (providerView.Controls.SwipeView.isPreviousItem)
				jumpToNextView()

			removeLoadingView()
			focusFieldViews()
		}

		function onRegistrationOutOfBandUrlReceived(outOfBandUrl) {
			providerView.outOfBandUrl = outOfBandUrl
			handleInBandRegistrationNotSupported()
		}

		// Depending on the error, the swipe view jumps to the view where the input should be corrected.
		// For all remaining errors, the swipe view jumps to the provider view.
		function onRegistrationFailed(error, errorMessage) {
			registrationErrorOccurred = true

			switch(error) {
			case RegistrationManager.InBandRegistrationNotSupported:
				handleInBandRegistrationNotSupported()
				break
			case RegistrationManager.UsernameConflict:
				requestRegistrationForm()
				handleUsernameConflictError()
				jumpToView(usernameView)
				break
			case RegistrationManager.PasswordTooWeak:
				requestRegistrationForm()
				passiveNotification(qsTr("The provider requires a stronger password."))
				jumpToView(passwordView)
				break
			case RegistrationManager.CaptchaVerificationFailed:
				requestRegistrationForm()
				showPassiveNotificationForCaptchaVerificationFailedError()
				jumpToView(customFormView)
				break
			case RegistrationManager.RequiredInformationMissing:
				requestRegistrationForm()
				if (customFormView) {
					showPassiveNotificationForRequiredInformationMissingError(errorMessage)
					jumpToView(customFormView)
				} else {
					showPassiveNotificationForUnknownError(errorMessage)
				}
				break
			default:
				requestRegistrationForm()
				showPassiveNotificationForUnknownError(errorMessage)
				jumpToView(providerView)
			}
		}
	}

	// Simulate the pressing of the currently clickable confirmation button.
	Keys.onPressed: {
		switch (event.key) {
		case Qt.Key_Return:
		case Qt.Key_Enter:
			if (resultView && resultView.Controls.SwipeView.isCurrentItem)
				resultView.registrationButton.clicked()
			else if (jumpingToViewsEnabled)
				navigationBar.nextButton.clicked()
		}
	}

	/**
	 * Shows a passive notification regarding the missing support of In-Band Registration.
	 * If the provider supports web registration, the corresponding view is opened.
	 * If the provider does not support web registration and it is not a custom provider, another one is automatically selected.
	 */
	function handleInBandRegistrationNotSupported() {
		let notificationText = providerView.customProviderSelected ? qsTr("The provider does not support registration via this app.") : qsTr("The provider does currently not support registration via this app.")

		if (providerView.registrationWebPage.toString() || providerView.outOfBandUrl.toString()) {
			addWebRegistrationView()
			notificationText += " " + qsTr("But you can use the provider's web registration.")
		} else {
			if (!providerView.customProviderSelected) {
				providerView.selectProviderRandomly()
				notificationText += " " + qsTr("A new provider has been randomly selected.")
			}

			jumpToPreviousView()
		}

		passiveNotification(notificationText)
		removeLoadingView()
	}

	/**
	 * Shows a passive notification if a username is already taken on the provider.
	 * If a random username was used for registration, a new one is generated.
	 */
	function handleUsernameConflictError() {
		let notificationText = qsTr("The username is already taken.")

		if (usernameView.enteredText.length === 0) {
			usernameView.regenerateUsername()
			notificationText += " " + qsTr("A new random username has been generated.")
		}

		passiveNotification(notificationText)
	}

	/**
	 * Focuses the input field of the currently shown field view.
	 *
	 * This is necessary to execute after a registration form is received because the normal focussing within FieldView does not work then.
	 */
	function focusFieldViews() {
		if (swipeView.currentItem === usernameView || swipeView.currentIndex === passwordView || swipeView.currentIndex === customFormView)
			swipeView.currentItem.forceActiveFocus()
	}

	/**
	 * Adds the web registration view to the swipe view.
	 */
	function addWebRegistrationView() {
		removeDynamicallyLoadedInBandRegistrationViews()

		webRegistrationView = webRegistrationViewComponent.createObject(swipeView)
		swipeView.insertItem(providerView.Controls.SwipeView.index + 1, webRegistrationView)
	}

	/**
	 * Removes the web registration view from the swipe view.
	 */
	function removeWebRegistrationView() {
		if (webRegistrationView) {
			swipeView.removeItem(webRegistrationView)
			addDynamicallyLoadedInBandRegistrationViews()
		}
	}

	/**
	 * Adds the dynamically loaded views used for the In-Band Registration to the swipe view.
	 */
	function addDynamicallyLoadedInBandRegistrationViews() {
		let indexToInsert = providerView.Controls.SwipeView.index

		addUsernameView(++indexToInsert)
		addPasswordView(++indexToInsert)
		addCustomFormView(++indexToInsert)
		addResultView(++indexToInsert)
	}

	/**
	 * Removes the dynamically loaded views from the swipe view.
	 */
	function removeDynamicallyLoadedInBandRegistrationViews() {
		for (const view of [usernameView, passwordView, customFormView, resultView]) {
			swipeView.removeItem(view)
		}
	}

	/**
	 * Adds the loading view to the swipe view.
	 *
	 * @param index index of the swipe view at which the loading view will be inserted
	 */
	function addLoadingView(index) {
		loadingView = loadingViewComponent.createObject(swipeView)
		swipeView.insertItem(index, loadingView)
	}

	/**
	 * Removes the loading view from the swipe view after jumping to the next page.
	 */
	function removeLoadingView() {
		swipeView.removeItem(loadingView)
	}

	/**
	 * Adds the username view to the swipe view.
	 *
	 * @param index position in the swipe view to insert the username view
	 */
	function addUsernameView(index) {
		usernameView = usernameViewComponent.createObject(swipeView)
		swipeView.insertItem(index, usernameView)
	}

	/**
	 * Adds the password view to the swipe view.
	 *
	 * @param index position in the swipe view to insert the password view
	 */
	function addPasswordView(index) {
		passwordView = passwordViewComponent.createObject(swipeView)
		swipeView.insertItem(index, passwordView)
	}

	/**
	 * Adds the custom form view to the swipe view.
	 *
	 * @param index position in the swipe view to insert the custom form view
	 */
	function addCustomFormView(index) {
		customFormView = customFormViewComponent.createObject(swipeView)
		swipeView.insertItem(index, customFormView)
	}

	/**
	 * Adds the result view to the swipe view.
	 *
	 * @param index position in the swipe view to insert the result view
	 */
	function addResultView(index) {
		resultView = resultViewComponent.createObject(swipeView)
		swipeView.insertItem(index, resultView)
	}

	/**
	 * Jumps to the previous view.
	 */
	function jumpToPreviousView() {
		swipeView.decrementCurrentIndex()
	}

	/**
	 * Jumps to the next view.
	 */
	function jumpToNextView() {
		swipeView.incrementCurrentIndex()
	}

	/**
	 * Jumps to a given view.
	 *
	 * @param view view to be jumped to
	 */
	function jumpToView(view) {
		swipeView.setCurrentIndex(view.Controls.SwipeView.index)
	}

	/**
	 * Requests a registration and shows the loading view.
	 */
	function sendRegistrationFormAndShowLoadingView() {
		addLoadingView(swipeView.currentIndex + 1)
		jumpToNextView()

		Kaidan.client.vCardManager.changeNicknameRequested(displayName)
		sendRegistrationForm()
	}
}
