// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
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
 * This view is used for choosing a provider.
 */
RegistrationRequestPage {
	id: root
	title: qsTr("Choose a Provider") + " (" + (providerListView.count - 1) + " / " + (filterModel.sourceModel.rowCount() - 1) + ")"
	onBackRequested: account.settings.resetCustomConnectionSettings()
	actions: [
		Kirigami.Action {
			text: qsTr("Filter")
			icon.name: "filter-symbolic"
			displayHint: Kirigami.DisplayHint.IconOnly
			onTriggered: openView(providerFilterDialog, providerFilterPage)
		}
	]

	ListView {
		id: providerListView

		property BusyIndicatorFormButton lastClickedButton

		model: ProviderFilterModel {
			id: filterModel
			sourceModel: ProviderModel {}
		}
		spacing: Kirigami.Units.smallSpacing
		leftMargin: root.horizontalPadding
		rightMargin: leftMargin
		// Ensure that no expanded providerContentArea is removed when it goes too far out of the
		// view's visible area.
		// Otherwise, it would be collapsed.
		cacheBuffer: Kirigami.Units.gridUnit * 100 * count
		header: Kirigami.Heading {
			text: qsTr("Choose a provider to register!")
			level: 1
			wrapMode: Text.Wrap
			horizontalAlignment: Text.AlignHCenter
			verticalAlignment: Text.AlignVCenter
			width: parent.width
			height: Kirigami.Units.gridUnit * 4
		}
		delegate: FormCard.FormCard {
			id: providerDelegate
			width: ListView.view.width - ListView.view.leftMargin * 2

			FormCard.AbstractFormDelegate {
				topPadding: 0
				bottomPadding: 0
				leftPadding: 0
				rightPadding: 0
				background: null
				contentItem: Column {
					spacing: 0

					FormCard.FormTextDelegate {
						id: providerExpansionButton
						text: model.display
						font.italic: model.isCustomProvider
						width: parent.width
						checkable: true
						background: FormExpansionButtonBackground {
							card: providerDelegate
							button: providerExpansionButton
						}
						trailing: FormCard.FormArrow {
							direction: providerExpansionButton.checked ? Qt.UpArrow : Qt.DownArrow
						}
					}

					Controls.Control {
						id: providerContentArea
						width: parent.width
						height: providerExpansionButton.checked ? implicitHeight : 0
						clip: true
						leftPadding: 0
						rightPadding: 0
						topPadding: 0
						bottomPadding: 0
						contentItem: ColumnLayout {
							spacing: 0

							Kirigami.Separator {
								Layout.fillWidth: true
							}

							Loader {
								id: contentAreaLoader
								sourceComponent: model.isCustomProvider ? customProviderArea : regularProviderArea

								Component {
									id: customProviderArea

									ColumnLayout {
										spacing: 0
										width: providerContentArea.width

										property alias providerField: providerField

										FormCard.FormTextFieldDelegate {
											id: providerField
											label: qsTr("Provider Address")
											placeholderText: "example.org"
											inputMethodHints: Qt.ImhUrlCharactersOnly
											onAccepted: {
												if (customConnectionSettings.visible) {
													customConnectionSettings.forceActiveFocus()
												} else {
													choiceButton.clicked()
												}
											}
											trailing: ClickableIcon {
												Controls.ToolTip.text: qsTr("Show connection settings")
												source: "preferences-system-symbolic"
												implicitWidth: Kirigami.Units.iconSizes.small
												implicitHeight: Kirigami.Units.iconSizes.small
												Layout.leftMargin: Kirigami.Units.mediumSpacing
												onClicked: {
													customConnectionSettings.visible = !customConnectionSettings.visible

													if (customConnectionSettings.visible) {
														customConnectionSettings.forceActiveFocus()
													} else {
														providerField.forceActiveFocus()
													}
												}
											}
										}

										CustomConnectionSettings {
											id: customConnectionSettings
											accountSettings: root.account.settings
											confirmationButton: choiceButton
											visible: false
										}

										Connections {
											target: pageStack.layers

											function onCurrentItemChanged() {
												const currentItem = pageStack.layers.currentItem

												// Focus providerField on first opening and when going back from sub pages (i.e., layers above).
												if (currentItem === root) {
													providerField.forceActiveFocus()
												}
											}
										}
									}
								}

								Component {
									id: regularProviderArea

									ColumnLayout {
										spacing: 0
										width: providerContentArea.width

										FormCard.FormTextDelegate {
											text: qsTr("Data storage/processing locations")
											description: model.flagsText
										}

										FormCard.FormTextDelegate {
											text: qsTr("Languages")
											description: model.languages
										}

										FormCard.FormTextDelegate {
											text: qsTr("Available/Listed since")
											description: model.sinceText
										}

										FormCard.FormTextDelegate {
											text: qsTr("Maximum size of shared media")
											description: model.httpUploadFileSizeText
										}

										FormCard.FormTextDelegate {
											text: qsTr("Message storage duration")
											description: model.messageStorageDurationText
										}

										UrlFormButtonDelegate {
											text: qsTr("Visit details page")
											description: qsTr("Open the provider's details web page")
											icon.name: "globe"
											url: Utils.providerDetailsUrl(model.jid)
										}
									}
								}
							}

							Kirigami.Separator {
								Layout.fillWidth: true
							}

							BusyIndicatorFormButton {
								id: choiceButton
								idleText: qsTr("Choose")
								busyText: qsTr("Requesting registration…")
								busy: root.account.connection.state === Enums.StateConnecting && providerListView.lastClickedButton === this
								background: HighlightedFormButtonBackground {
									corners {
										topLeftRadius: 0
										topRightRadius: 0
										bottomLeftRadius: providerDelegate.cardWidthRestricted ? Kirigami.Units.smallSpacing : 0
										bottomRightRadius: providerDelegate.cardWidthRestricted ? Kirigami.Units.smallSpacing : 0
									}
								}
								onClicked: {
									providerListView.lastClickedButton = this

									if (model.isCustomProvider) {
										const loadedCustomProviderArea = contentAreaLoader.item
										const providerField = loadedCustomProviderArea.providerField
										const chosenProvider = providerField.text

										if (chosenProvider) {
											root.account.settings.jid = chosenProvider
											requestRegistrationForm()
										} else {
											passiveNotification(qsTr("You must enter a provider address"))
											providerField.forceActiveFocus()
										}
									} else {
										root.account.settings.jid = model.jid
										root.account.settings.resetCustomConnectionSettings()

										if (model.supportsInBandRegistration) {
											requestRegistrationForm()
										} else {
											openWebRegistrationPage(model.registrationWebPage)
										}
									}
								}
							}
						}

						Behavior on height {
							SmoothedAnimation {
								duration: Kirigami.Units.veryLongDuration
							}
						}
					}
				}
			}
		}
	}

	Component {
		id: providerFilterDialog

		Dialog {
			title: qsTr("Filter")
			bottomInset: - Kirigami.Units.cornerRadius

			ProviderFilterArea {
				providerFilterModel: filterModel
			}
		}
	}

	Component {
		id: providerFilterPage

		Kirigami.ScrollablePage {
			title: qsTr("Filter")
			background: Rectangle {
				color: Kirigami.Theme.alternateBackgroundColor
			}
			bottomPadding: 0

			ProviderFilterArea {
				providerFilterModel: filterModel
			}
		}
	}

	Component {
		id: manualRegistrationPage

		ManualRegistrationPage {
			account: root.account
		}
	}

	Component {
		id: webRegistrationPage

		WebRegistrationPage {
			account: root.account
		}
	}

	Connections {
		target: root.account.registrationController
		enabled: pageStack.layers.currentItem === root

		function onRegistrationFormReceived(dataFormModel) {
			let page = pushLayer(manualRegistrationPage)
			page.account = root.account
			page.formModel = dataFormModel
			page.formUpdated()
		}
	}

	Connections {
		target: root.account.registrationController

		function onRegistrationOutOfBandUrlReceived(outOfBandUrl) {
			openWebRegistrationPage(outOfBandUrl)
		}

		function onRegistrationFailed(error, errorMessage) {
			switch(error) {
			case RegistrationController.InBandRegistrationNotSupported:
				passiveNotification(qsTr("The provider does not support registration via this app."))
				break
			case RegistrationController.TemporarilyBlocked:
				passiveNotification(qsTr("You are temporarily blocked: ") + errorMessage)
				break
			}
		}
	}

	function openWebRegistrationPage(registrationWebPage) {
		let page = pushLayer(webRegistrationPage)
		page.account = account
		page.registrationWebPage = registrationWebPage
	}
}
