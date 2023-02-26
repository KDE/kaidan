// SPDX-FileCopyrightText: 2023 Mathis Brüchert <mbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.12 as Kirigami
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

import im.kaidan.kaidan 1.0

import ".."

ColumnLayout {
	id: root
	spacing: Kirigami.Units.largeSpacing
	Layout.topMargin: Kirigami.Units.largeSpacing

	property Kirigami.OverlaySheet sheet
	required property string jid
	required property ColumnLayout encryptionArea
	property alias qrCodePage: qrCodePage

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
		visible: infoRepeater.count
		Layout.fillWidth: true
		contentItem: ColumnLayout {
			spacing: 0

			MobileForm.FormCardHeader {
				title: qsTr("Profile")
			}

			Repeater {
				id: infoRepeater
				Layout.fillHeight: true
				model: VCardModel {
					jid: root.jid
				}
				delegate: MobileForm.AbstractFormDelegate {
					Layout.fillWidth: true
					background: Item {}
					contentItem: ColumnLayout {
						Controls.Label {
							text: Utils.formatMessage(model.value)
							textFormat: Text.StyledText
							wrapMode: Text.WordWrap
							Layout.fillWidth: true
							onLinkActivated: Qt.openUrlExternally(link)
						}

						Controls.Label {
							text: model.key
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

	MobileForm.FormCard {
		Layout.fillWidth: true
		contentItem: encryptionArea
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
					Layout.fillWidth: true
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
