// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
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
import "../settings"

/**
 * This view is used for choosing a provider.
 */
RegistrationPage {
	id: root
	title: qsTr("Choose a Provider")
	onBackRequested: resetCustomConnectionSettings()

	ListView {
		id: providerListView

		property BusyIndicatorFormButton lastClickedButton

		model: ProviderListModel {}
		spacing: Kirigami.Units.smallSpacing
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
		delegate: MobileForm.FormCard {
			id: providerDelegate
			width: ListView.view.width
			contentItem: Column {
				spacing: 0

				MobileForm.FormTextDelegate {
					id: providerExpansionButton
					text: model.jid
					font.italic: index === 0
					width: parent.width
					background: MobileForm.FormDelegateBackground {
						control: providerExpansionButton
					}
					trailing: MobileForm.FormArrow {
						direction: providerContentArea.active ? MobileForm.FormArrow.Up : MobileForm.FormArrow.Down
					}
					onClicked: providerContentArea.active = !providerContentArea.active
				}

				Controls.Control {
					id: providerContentArea

					property bool active: false

					width: parent.width
					height: active ? implicitHeight : 0
					visible: height != 0
					clip: true
					leftPadding: 0
					rightPadding: 0
					topPadding: 0
					bottomPadding: 0
					background: Rectangle {
						color: secondaryBackgroundColor
					}
					contentItem: ColumnLayout {
						spacing: 0

						Kirigami.Separator {
							Layout.fillWidth: true
						}

						Loader {
							id: contentAreaLoader
							sourceComponent: index === 0 ? customProviderArea : regularProviderArea

							Component {
								id: customProviderArea

								MobileForm.AbstractFormDelegate {
									property alias providerField: providerField
									property alias hostField: customConnectionSettings.hostField
									property alias portField: customConnectionSettings.portField

									width: providerContentArea.width
									background: null
									contentItem: ColumnLayout {
										spacing: 0

										Field {
											id: providerField
											labelText: qsTr("Provider Address")
											placeholderText: "example.org"
											inputMethodHints: Qt.ImhUrlCharactersOnly
											inputField.onAccepted: {
												if (customConnectionSettings.visible) {
													customConnectionSettings.forceActiveFocus()
												} else {
													choiceButton.clicked()
												}
											}
											inputField.rightActions: [
												Kirigami.Action {
													icon.name: "preferences-system-symbolic"
													text: qsTr("Connection settings")
													onTriggered: {
														customConnectionSettings.visible = !customConnectionSettings.visible

														if (customConnectionSettings.visible) {
															customConnectionSettings.forceActiveFocus()
														} else {
															providerField.forceActiveFocus()
														}
													}
												}
											]
										}

										CustomConnectionSettings {
											id: customConnectionSettings
											confirmationButton: choiceButton
											visible: false
											Layout.topMargin: Kirigami.Units.largeSpacing
										}

										Connections {
											target: pageStack.layers

											function onCurrentItemChanged() {
												const currentItem = pageStack.layers.currentItem

												// Focus providerField.inputField on first opening
												// and when going back from sub pages (i.e., layers
												// above).
												if (currentItem === root) {
													providerField.inputField.forceActiveFocus()
												}
											}
										}
									}
								}
							}

							Component {
								id: regularProviderArea

								ColumnLayout {
									width: providerContentArea.width
									spacing: 0

									MobileForm.FormTextDelegate {
										text: model.countries
										description: qsTr("Server locations")
									}

									MobileForm.FormTextDelegate {
										text: model.languages
										description: qsTr("Languages")
									}

									MobileForm.FormTextDelegate {
										text: model.since
										description: qsTr("Available/Listed since")
									}

									MobileForm.FormTextDelegate {
										text: model.httpUploadSize
										description: qsTr("Maximum size of shared media")
									}

									MobileForm.FormTextDelegate {
										text: model.messageStorageDuration
										description: qsTr("Messages storage duration")
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

						MobileForm.FormCard {
							Layout.topMargin: Kirigami.Units.gridUnit
							Layout.bottomMargin: Layout.topMargin
							Layout.leftMargin: Layout.topMargin
							Layout.rightMargin: Layout.topMargin
							Layout.fillWidth: true
							Kirigami.Theme.colorSet: Kirigami.Theme.Selection
							contentItem: BusyIndicatorFormButton {
								id: choiceButton
								idleText: qsTr("Choose")
								busyText: qsTr("Requesting registration…")
								busy: Kaidan.connectionState === Enums.StateConnecting && providerListView.lastClickedButton === this
								onClicked: {
									providerListView.lastClickedButton = this

									if (index === 0) {
										const loadedCustomProviderArea = contentAreaLoader.item
										const providerField = loadedCustomProviderArea.providerField

										const chosenProvider = providerField.text

										if (chosenProvider) {
											root.provider = chosenProvider

											AccountManager.host = loadedCustomProviderArea.hostField.text
											AccountManager.port = loadedCustomProviderArea.portField.value

											requestRegistrationForm()
										} else {
											passiveNotification(qsTr("You must enter a provider address"))
											providerField.forceActiveFocus()
										}
									} else {
										root.provider = model.jid
										resetCustomConnectionSettings()

										if (model.supportsInBandRegistration) {
											requestRegistrationForm()
										} else {
											openWebRegistrationPage(model.registrationWebPage)
										}
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

	Component {
		id: manualRegistrationPage

		ManualRegistrationPage {}
	}

	Component {
		id: webRegistrationPage

		WebRegistrationPage {}
	}

	Connections {
		target: Kaidan
		enabled: pageStack.layers.currentItem === root

		function onRegistrationFormReceived(dataFormModel) {
			let page = pushLayer(manualRegistrationPage)
			page.provider = root.provider
			page.processFormModel(dataFormModel)
		}
	}

	Connections {
		target: Kaidan

		function onRegistrationOutOfBandUrlReceived(outOfBandUrl) {
			openWebRegistrationPage(outOfBandUrl)
		}

		function onRegistrationFailed(error, errorMessage) {
			switch(error) {
			case RegistrationManager.InBandRegistrationNotSupported:
				passiveNotification(qsTr("The provider does not support registration via this app."))
				break
			case RegistrationManager.TemporarilyBlocked:
				showPassiveNotificationForUnknownError(errorMessage)
				break
			}
		}
	}

	function openWebRegistrationPage(registrationWebPage) {
		let page = pushLayer(webRegistrationPage)
		page.provider = provider
		page.registrationWebPage = registrationWebPage
	}

	/**
	 * Resets possibly set custom connection settings.
	 */
	function resetCustomConnectionSettings() {
		AccountManager.host = ""
		AccountManager.port = AccountManager.portAutodetect
	}
}
