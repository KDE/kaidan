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
 * This page is used for creating an account via web registration.
 */
RegistrationPage {
	id: root

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

		MobileForm.FormCard {
			Layout.fillWidth: true
			contentItem: ColumnLayout {
				spacing: 0

				MobileForm.FormCardHeader {
					title: qsTr("Step 1: Register account")
				}

				MobileForm.FormTextDelegate {
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
		}

		MobileForm.FormCard {
			id: loginFormCard
			visible: false
			Layout.fillWidth: true
			contentItem: ColumnLayout {
				spacing: 0

				MobileForm.FormCardHeader {
					title: qsTr("Step 2: Log in with your registered account")
				}

				LoginArea {
					id: loginArea
				}
			}
		}
	}

	function showLoginArea() {
		AccountManager.jid = provider
		loginFormCard.visible = true
		loginArea.initialize()
	}
}
