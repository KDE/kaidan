// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Bhavy Airi <airiraghav@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14

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
			}
		}

		CenteredAdaptiveButton {
			text: qsTr("Copy registration web page address")
			onClicked: Utils.copyToClipboard(providerView.registrationWebPage.toString() ? providerView.registrationWebPage : providerView.outOfBandUrl)
		}

		CenteredAdaptiveHighlightedButton {
			id: loginButton
			text: qsTr("Log in with your new account")
			Layout.topMargin: height
			onClicked: {
				AccountManager.jid = providerView.text
				popLayersAboveLowest()
			}
		}
	}
}
