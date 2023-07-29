// SPDX-FileCopyrightText: 2023 Mathis Brüchert <mbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

import im.kaidan.kaidan 1.0

import ".."
import "../elements"

Controls.Control {
	id: root

	default property alias __data: mainArea.data
	property Kirigami.OverlaySheet sheet
	required property string jid
	property alias qrCodePage: qrCodePage
	property alias mediaOverview: mediaOverview
	property alias mediaOverviewExpansionButton: mediaOverviewExpansionButton
	property alias vCardArea: vCardArea.data
	property alias vCardRepeater: vCardRepeater
	property ColumnLayout rosterGroupArea
	required property ColumnLayout encryptionArea

	topPadding: Kirigami.Settings.isMobile ? Kirigami.Units.largeSpacing : Kirigami.Units.largeSpacing * 3
	bottomPadding: Kirigami.Settings.isMobile ? 0 : Kirigami.Units.largeSpacing * 3
	leftPadding: bottomPadding
	rightPadding: leftPadding
	background: Rectangle {
		color: secondaryBackgroundColor
	}

	contentItem: ColumnLayout {
		id: mainArea
		spacing: Kirigami.Units.largeSpacing

		Component {
			id: qrCodePage

			QrCodePage {
				Component.onCompleted: {
					if (root.sheet) {
						root.sheet.close()
					}
				}

				Component.onDestruction: {
					if (root.sheet) {
						root.sheet.open()
					}
				}
			}
		}

		MobileForm.FormCard {
			visible: contentItem.enabled
			Layout.fillWidth: true
			contentItem: ColumnLayout {
				enabled: mediaOverview.totalFilesCount
				spacing: 0

				MobileForm.FormCardHeader {
					title: qsTr("Media")
				}

				MediaOverview {
					id: mediaOverview
					visible: mediaOverviewExpansionButton.checked
					Layout.fillWidth: true
				}

				FormExpansionButton {
					id: mediaOverviewExpansionButton
					onCheckedChanged: {
						if (checked) {
							mediaOverview.selectionMode = false

							// Display the content of the first tab only on initial loading.
							// Afterwards, display the content of the last active tab.
							if (mediaOverview.tabBarCurrentIndex === -1) {
								mediaOverview.tabBarCurrentIndex = 0
							}

							mediaOverview.loadDownloadedFiles()
						}
					}
				}
			}
		}

		MobileForm.FormCard {
			visible: vCardRepeater.count
			Layout.fillWidth: true
			contentItem: ColumnLayout {
				id: vCardArea
				spacing: 0

				MobileForm.FormCardHeader {
					title: qsTr("Profile")
				}

				Repeater {
					id: vCardRepeater
					Layout.fillHeight: true
				}
			}
		}

		MobileForm.FormCard {
			Layout.fillWidth: true
			contentItem: root.rosterGroupArea
		}

		MobileForm.FormCard {
			Layout.fillWidth: true
			contentItem: root.encryptionArea
		}

		MobileForm.FormCard {
			visible: deviceRepeater.count
			Layout.fillWidth: true
			contentItem: ColumnLayout {
				spacing: 0

				MobileForm.FormCardHeader {
					title: qsTr("Connected Devices")
				}

				Repeater {
					id: deviceRepeater
					Layout.fillHeight: true
					model: UserDevicesModel {
						jid: root.jid
					}
					delegate: MobileForm.AbstractFormDelegate {
						background: Item {}
						contentItem: ColumnLayout {
							Controls.Label {
								text: {
									if (model.name) {
										if (model.version) {
											return model.name + " " + model.version
										}
										return model.name
									}
									return model.resource
								}
								textFormat: Text.PlainText
								wrapMode: Text.WordWrap
								Layout.fillWidth: true
							}

							Controls.Label {
								text: model.os
								color: Kirigami.Theme.disabledTextColor
								font: Kirigami.Theme.smallFont
								textFormat: Text.PlainText
								wrapMode: Text.WordWrap
								Layout.fillWidth: true
							}
						}
					}
				}
			}
		}
	}
}
