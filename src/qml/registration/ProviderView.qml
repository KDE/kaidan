// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

import "../elements"
import "../elements/fields"
import "../settings"

/**
 * This view is used for choosing a provider.
 */
FieldView {
	descriptionText: qsTr("The provider is where your account is located.\nThe selectable providers are hand-picked by our community!")
	imageSource: "provider"

	property string text: customProviderSelected ? field.text : providerListModel.data(comboBox.currentIndex, ProviderListModel.JidRole)
	property string website: providerListModel.data(comboBox.currentIndex, ProviderListModel.WebsiteRole)
	property bool customProviderSelected: providerListModel.data(comboBox.currentIndex, ProviderListModel.IsCustomProviderRole)
	property bool inBandRegistrationSupported: providerListModel.data(comboBox.currentIndex, ProviderListModel.SupportsInBandRegistrationRole)
	property string registrationWebPage: providerListModel.data(comboBox.currentIndex, ProviderListModel.RegistrationWebPageRole)
	property bool shouldWebRegistrationViewBeShown: !customProviderSelected && !inBandRegistrationSupported
	property string outOfBandUrl

	property alias customConnectionSettings: customConnectionSettings

	ColumnLayout {
		parent: contentArea
		spacing: Kirigami.Units.largeSpacing

		Controls.Label {
			text: qsTr("Provider")
		}

		Controls.ComboBox {
			id: comboBox
			Layout.fillWidth: true
			model: ProviderListModel {
				id: providerListModel
			}
			textRole: "display"
			currentIndex: indexOfRandomlySelectedProvider()
			onCurrentIndexChanged: field.text = ""

			onActivated: {
				if (index === 0) {
					editText = ""

					// Focus the whole combo box.
					forceActiveFocus()

					// Focus the input text field of the combo box.
					nextItemInFocusChain().forceActiveFocus()
				} else if (customConnectionSettings.visible) {
					customConnectionSettings.visible = false
				}
			}
		}

		Field {
			id: field
			visible: customProviderSelected
			placeholderText: "example.org"
			inputMethodHints: Qt.ImhUrlCharactersOnly

			inputField.rightActions: [
				Kirigami.Action {
					icon.name: "preferences-system-symbolic"
					text: qsTr("Connection settings")
					onTriggered: {
						customConnectionSettings.visible = !customConnectionSettings.visible

						if (customConnectionSettings.visible)
							customConnectionSettings.forceActiveFocus()
					}
				}
			]

			onTextChanged: {
				if (outOfBandUrl && customProviderSelected) {
					outOfBandUrl = ""
					removeWebRegistrationView()
				}
			}

			// Focus the customConnectionSettings on confirmation.
			Keys.onPressed: {
				if (customConnectionSettings.visible) {
					switch (event.key) {
					case Qt.Key_Return:
					case Qt.Key_Enter:
						customConnectionSettings.forceActiveFocus()
						event.accepted = true
					}
				}
			}
		}

		CustomConnectionSettings {
			id: customConnectionSettings
			confirmationButton: navigationBar.nextButton
			visible: false
		}

		Controls.ScrollView {
			Layout.fillWidth: true
			Layout.fillHeight: true

			Kirigami.FormLayout {
				Controls.Label {
					visible: !customProviderSelected && text
					Kirigami.FormData.label: qsTr("Web registration only:")
					text: inBandRegistrationSupported ? qsTr("No") : qsTr("Yes")
				}

				Controls.Label {
					visible: !customProviderSelected && text
					Kirigami.FormData.label: qsTr("Server locations:")
					text: providerListModel.data(comboBox.currentIndex, ProviderListModel.CountriesRole)
				}

				Controls.Label {
					visible: !customProviderSelected && text
					Kirigami.FormData.label: qsTr("Languages:")
					text: providerListModel.data(comboBox.currentIndex, ProviderListModel.LanguagesRole)
				}

				Controls.Label {
					visible: !customProviderSelected && text
					Kirigami.FormData.label: qsTr("Online since:")
					text: providerListModel.data(comboBox.currentIndex, ProviderListModel.OnlineSinceRole)
				}

				Controls.Label {
					visible: !customProviderSelected && text
					Kirigami.FormData.label: qsTr("Allows to share media up to:")
					text: providerListModel.data(comboBox.currentIndex, ProviderListModel.HttpUploadSizeRole)
				}

				Controls.Label {
					visible: !customProviderSelected && text
					Kirigami.FormData.label: qsTr("Stores shared media up to:")
					text: providerListModel.data(comboBox.currentIndex, ProviderListModel.MessageStorageDurationRole)
				}
			}
		}

		CenteredAdaptiveHighlightedButton {
			visible: !customProviderSelected
			text: qsTr("Open website")
			onClicked: Qt.openUrlExternally(website)
		}

		CenteredAdaptiveButton {
			visible: !customProviderSelected
			text: qsTr("Copy website address")
			onClicked: Utils.copyToClipboard(website)
		}

		// placeholder
		Item {
			Layout.fillHeight: true
		}
	}

	onShouldWebRegistrationViewBeShownChanged: {
		// Show the web registration view for non-custom providers if only web registration is supported or hides the view otherwise.
		if (shouldWebRegistrationViewBeShown)
			addWebRegistrationView()
		else
			removeWebRegistrationView()
	}

	/**
	 * Randomly sets a new provider as selected for registration.
	 */
	function selectProviderRandomly() {
		comboBox.currentIndex = indexOfRandomlySelectedProvider()
	}

	/**
	 * Returns the index of a randomly selected provider for registration.
	 */
	function indexOfRandomlySelectedProvider() {
		return providerListModel.randomlyChooseIndex()
	}
}
