// SPDX-FileCopyrightText: 2023 Mathis Brüchert <mbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan

import "../elements"

FormCard.FormCard {
	id: root

	required property Account account
	required property string jid

	visible: account.settings.enabled && deviceRepeater.count
	Layout.fillWidth: true

	FormCard.FormHeader {
		title: qsTr("Connected Devices")
	}

	Repeater {
		id: deviceRepeater
		Layout.fillHeight: true
		model: UserDevicesModel {
			versionController: root.account.versionController
			presenceCache: root.account.presenceCache
			jid: root.jid
		}
		delegate: FormCard.AbstractFormDelegate {
			visible: deviceExpansionButton.checked
			background: null
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

	FormExpansionButton {
		id: deviceExpansionButton
	}
}
