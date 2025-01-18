// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Bhavy Airi <airiraghav@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan

import ".."
import "../elements"

/**
 * This page is used for creating an account via web registration.
 */
ImageBackgroundPage {
	id: root

	property string provider
	property url registrationWebPage

	title: qsTr("Web Registration")

	ColumnLayout {
		Kirigami.Heading {
			text: qsTr("Register via the provider's website!")
			level: 1
			wrapMode: Text.Wrap
			horizontalAlignment: Text.AlignHCenter
			Layout.leftMargin: Kirigami.Units.largeSpacing
			Layout.rightMargin: Layout.leftMargin
			Layout.fillWidth: true
		}

		Image {
			source: Utils.getResourcePath("images/onboarding/web-registration.svg")
			fillMode: Image.PreserveAspectFit
			Layout.alignment: Qt.AlignHCenter
			Layout.fillWidth: true
		}

		FormCard.FormCard {
			Layout.fillWidth: true

			FormCard.FormHeader {
				title: qsTr("Step 1: Register account")
			}

			FormCard.FormTextDelegate {
				text: qsTr("The selected provider only supports web registration")
				description: qsTr("After creating an account via the provider's website, you can log in via Kaidan")
			}

			UrlFormButtonDelegate {
				text: qsTr("Visit registration page")
				description: qsTr("Open the provider's registration web page")
				icon.name: "globe"
				url: root.registrationWebPage
				onClicked: root.showLoginArea()
				copyButton.onClicked: root.showLoginArea()
			}
		}

		FormCard.FormCard {
			id: loginFormCard
			visible: false
			Layout.fillWidth: true

			FormCard.FormHeader {
				title: qsTr("Step 2: Log in with your registered account")
			}

			LoginArea {
				id: loginArea
			}
		}
	}

	function showLoginArea() {
		AccountManager.jid = provider
		loginFormCard.visible = true
		loginArea.initialize()
	}
}
