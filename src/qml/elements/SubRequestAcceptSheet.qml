// SPDX-FileCopyrightText: 2018 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

Kirigami.OverlaySheet {
	property string from;

	header: Kirigami.Heading {
		text: qsTr("Subscription Request")
	}

	contentItem: ColumnLayout {
		Layout.preferredWidth: 400

		spacing: 10

		Controls.Label {
			text: qsTr("You received a subscription request by <b>%1</b>. " +
				"If you accept it, the account will have access to your " +
				"presence status.").arg(from)
			wrapMode: Text.WordWrap
			Layout.fillWidth: true
		}

		RowLayout {
			Layout.topMargin: 10

			Button {
				Layout.fillWidth: true
				text: qsTr("Decline")
				onClicked: {
					Kaidan.client.rosterManager.answerSubscriptionRequestRequested(from, false)
					close()
				}
			}

			Button {
				Layout.fillWidth: true
				text: qsTr("Accept")
				onClicked: {
					Kaidan.client.rosterManager.answerSubscriptionRequestRequested(from, true)
					close()
				}
			}
		}
	}
}
