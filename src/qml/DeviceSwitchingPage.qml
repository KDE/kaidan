// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2020 Mathis Brüchert <mbblp@protonmail.ch>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

import "elements"

/**
 * This page shows the user's credentials as a QR code or as plain text.
 *
 * It enables the user to log in on another device.
 */
ExplanationOptionsTogglePage {
	title: qsTr("Switch device")
	primaryButton.text: state === "primaryAreaDisplayed" ? qsTr("Hide QR code") : qsTr("Show as QR code")
	secondaryButton.text: state === "secondaryAreaDisplayed" ? qsTr("Hide text") : qsTr("Show as text")
	explanation: ColumnLayout {
		width: parent.width
		height: parent.height

		Item {
			Layout.fillHeight: true
		}

		CenteredAdaptiveText {
			text: qsTr("Scan the QR code or enter the credentials as text on another device to log in on it.")
			scaleFactor: 1.5
		}

		CenteredAdaptiveText {
			text: qsTr("Attention:")
			color: Kirigami.Theme.neutralTextColor
			scaleFactor: 1.3
			Layout.topMargin: Kirigami.Units.largeSpacing * 2
		}

		CenteredAdaptiveText {
			text: qsTr("Never show this QR code to anyone else. It would allow unlimited access to your account!")
			color: Kirigami.Theme.neutralTextColor
			font.weight: Font.Light
			scaleFactor: 1.3
		}

		Item {
			Layout.fillHeight: true
		}
	}
	explanationAreaBackground.opacity: 1
	primaryArea: LoginQrCode {
		jid: AccountManager.jid
		width: Math.min(largeButtonWidth, parent.width, parent.height)
		height: width
		anchors.centerIn: parent
	}
	secondaryArea: Kirigami.FormLayout {
		anchors.centerIn: parent

		RowLayout {
			Kirigami.FormData.label: qsTr("Chat address:")
			Layout.fillWidth: true

			Controls.Label {
				text: AccountManager.jid
				Layout.fillWidth: true
			}

			Controls.ToolButton {
				text: qsTr("Copy chat address")
				icon.name: "edit-copy-symbolic"
				display: Controls.AbstractButton.IconOnly
				flat: true
				Layout.alignment: Qt.AlignRight
				onClicked: Utils.copyToClipboard(AccountManager.jid)
			}
		}

		RowLayout {
			Kirigami.FormData.label: qsTr("Password:")
			visible: Kaidan.settings.passwordVisibility === Kaidan.PasswordVisible
			Layout.fillWidth: true

			Controls.Label {
				text: AccountManager.password
				Layout.fillWidth: true
			}

			Controls.ToolButton {
				text: qsTr("Copy password")
				icon.name: "edit-copy-symbolic"
				display: Controls.AbstractButton.IconOnly
				flat: true
				Layout.alignment: Qt.AlignRight
				onClicked: Utils.copyToClipboard(AccountManager.password)
			}
		}
	}
}
