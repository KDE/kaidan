// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Bhavy Airi <airiraghav@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.19 as Kirigami
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

import im.kaidan.kaidan 1.0

import "../elements"

/**
 * This view is used for creating an account via web registration.
 */
View {
	descriptionText: qsTr("The selected provider only provides web registration. After creating an account via the web page, you can log in via Kaidan.")
	imageSource: "web-registration"

	ColumnLayout {
		parent: contentArea

		CenteredAdaptiveHighlightedButton {
			text: qsTr("Open registration web page")
			onClicked: {
				Qt.openUrlExternally(providerView.registrationWebPage.toString() ? providerView.registrationWebPage : providerView.outOfBandUrl)
				AccountManager.jid = providerView.text
				loginFormCard.visible = true
				loginArea.initialize()
			}
		}

		CenteredAdaptiveButton {
			text: qsTr("Copy registration web page address")
			onClicked: {
				Utils.copyToClipboard(providerView.registrationWebPage.toString() ? providerView.registrationWebPage : providerView.outOfBandUrl)
				AccountManager.jid = providerView.text
				loginFormCard.visible = true
				loginArea.initialize()
			}
		}

		MobileForm.FormCard {
			id: loginFormCard
			visible: false
			contentItem: ColumnLayout {
				spacing: 0

				MobileForm.FormCardHeader {
					title: qsTr("Log in with your new account")
				}

				LoginArea {
					id: loginArea
				}
			}
			Layout.alignment: Qt.AlignHCenter
			Layout.topMargin: Kirigami.Units.largeSpacing
			Layout.maximumWidth: largeButtonWidth
			Layout.fillWidth: true
		}
	}
}
