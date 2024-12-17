// SPDX-FileCopyrightText: 2023 Mathis Brüchert <mbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.19 as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan 1.0

import "../elements"

FormCard.FormCard {
	id: root

	required property string jid

	visible: deviceRepeater.count
	Layout.fillWidth: true

	FormCard.FormHeader {
		title: qsTr("Connected Devices")
	}

	Repeater {
		id: deviceRepeater
		Layout.fillHeight: true
		model: UserDevicesModel {
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
