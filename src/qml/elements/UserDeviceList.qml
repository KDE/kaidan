// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2023 Bhavy Airi <airiraghav@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14

import im.kaidan.kaidan 1.0

Repeater {
	property string userJid

	model: UserDevicesModel {
		jid: userJid
	}

	delegate: ColumnLayout {
		Controls.Label {
			text: {
				if (model.name) {
					let result = model.name

					if (model.version) {
						result += " v" + model.version
					}

					if (model.os) {
						result += " • " + model.os
					}

					return result
				}

				return model.resource
			}
			textFormat: Text.PlainText
		}
		Item {
			height: 3
		}
	}
}
