// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2023 Mathis Brüchert <mbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls
import QtMultimedia 5.14 as Multimedia
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0
import MediaUtils 0.1
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

SettingsPageBase {
	id: root

	property string title: qsTr("Multimedia Settings")

	implicitHeight: layout.implicitHeight
	implicitWidth: layout.implicitWidth
	MediaRecorder {
		id: recorder
	}

	ColumnLayout {
		id: layout

		Layout.preferredWidth: 600
		anchors.fill: parent

		MobileForm.FormCard {
			Layout.fillWidth: true
			contentItem: ColumnLayout {
				spacing: 0

				MobileForm.FormCardHeader {
					title: qsTr("Select Sources")
				}

				MobileForm.FormComboBoxDelegate {
					id: camerasComboBox
					text: qsTr('Camera')
					displayMode: MobileForm.FormComboBoxDelegate.Dialog
					dialog:Controls.Menu {
						z:60000
						width: parent.width

						id: camerasMenu
						Instantiator {
							id: camerasInstanciator
							model: recorder.cameraModel
							onObjectAdded: camerasMenu.insertItem(index, object)
							onObjectRemoved: camerasMenu.removeItem(object)
							delegate: Controls.MenuItem {
								required property string description
								required property string camera
								text: description
								onClicked: {
									recorder.mediaSettings.camera = camera
								}
							}
						}
					}

					displayText: camerasInstanciator.model.currentCamera.description
					Layout.fillWidth: true
				}
				MobileForm.FormComboBoxDelegate {
					id: audioInputsComboBox
					displayMode: MobileForm.FormComboBoxDelegate.Dialog

					text: qsTr('Audio Input')
					dialog:Controls.Menu {
						z:60000
						width: parent.width

						id: audioInputsMenu
						Instantiator {
							id:audioInputsInstanciator
							model: recorder.audioDeviceModel
							onObjectAdded: audioInputsMenu.insertItem(index, object)
							onObjectRemoved: audioInputsMenu.removeItem(object)
							delegate: Controls.MenuItem {
								required property string description
								required property int index
								text: description
								onClicked: {
									recorder.audioDeviceModel.currentIndex = index
								}
							}
						}
					}

					displayText: audioInputsInstanciator.model.currentAudioDevice.description
					Layout.fillWidth: true
				}
			}
		}
		MobileForm.FormCard {
			Layout.fillWidth: true
			contentItem: ColumnLayout {
				spacing: 0

				MobileForm.FormCardHeader {
					title: qsTr("Video Output")
				}
				Controls.ItemDelegate {
					id: item
					Layout.fillWidth: true
					Layout.fillHeight: true

					padding: 1

					hoverEnabled: true
					background: MobileForm.FormDelegateBackground {
						control: item
					}

					contentItem: Multimedia.VideoOutput {
						id: output
						source: recorder

						autoOrientation: true

						implicitWidth: contentRect.width < parent.width
									   && contentRect.height
									   < parent.height ? contentRect.width : parent.width
						implicitHeight: contentRect.width < parent.width
										&& contentRect.height
										< parent.height ? contentRect.height : parent.height
					}
				}
			}
		}
		Item {
			Layout.fillHeight: true
		}

		MobileForm.FormCard {
			id: card
			Layout.fillWidth: true

			contentItem: RowLayout {
				spacing: 0
				MobileForm.AbstractFormDelegate {
					Layout.fillWidth: true
					implicitWidth: (card.width / 3) - 1
					onClicked: recorder.resetSettingsToDefaults()
					contentItem: RowLayout {
						Kirigami.Icon {
							source: "kt-restore-defaults"
						}
						Controls.Label {
							Layout.fillWidth: true
							text: qsTr("Reset to defaults")
							wrapMode: Text.Wrap
						}
					}
				}

				Kirigami.Separator {
					Layout.fillHeight: true
				}
				MobileForm.AbstractFormDelegate {
					Layout.fillWidth: true
					implicitWidth: (card.width / 3) - 1
					onClicked: resetUserSettings()
					contentItem: RowLayout {
						Kirigami.Icon {
							source: "edit-reset"
						}
						Controls.Label {
							Layout.fillWidth: true
							text: qsTr("Reset User Settings")
							wrapMode: Text.Wrap
						}
					}
				}
				Kirigami.Separator {
					Layout.fillHeight: true
				}
				MobileForm.AbstractFormDelegate {
					Layout.fillWidth: true
					implicitWidth: (card.width / 3) - 1
					onClicked: {
						stack.pop()
						recorder.saveUserSettings()
					}
					contentItem: RowLayout {
						Kirigami.Icon {
							source: "document-save"
						}
						Controls.Label {
							Layout.fillWidth: true
							text: qsTr("Save")
							wrapMode: Text.Wrap
						}
					}
				}
			}
		}
	}

	Component.onCompleted: {
		recorder.type = MediaRecorder.Type.Image
	}
}
