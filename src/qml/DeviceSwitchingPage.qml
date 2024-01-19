// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2020 Mathis Brüchert <mbblp@protonmail.ch>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

import "elements"

/**
 * This page shows the user's credentials as a QR code or as plain text.
 *
 * It enables the user to log in on another device.
 */
ExplanationOptionsTogglePage {
	title: qsTr("Transfer account to another device")
	primaryButton.text: state === "primaryAreaDisplayed" ? qsTr("Hide QR code") : qsTr("Show as QR code")
	secondaryButton.text: state === "secondaryAreaDisplayed" ? qsTr("Hide text") : qsTr("Show as text")
	explanation: CenteredAdaptiveText {
		text: qsTr("Scan the QR code or enter the credentials as text on another device to log in on it.\n\nAttention:\nNever show this QR code to anyone else. It would allow unlimited access to your account!")
		verticalAlignment: Text.AlignVCenter
		Layout.fillHeight: true
		scaleFactor: 1.5
	}
	explanationAreaBackground.opacity: 1
	primaryArea: QrCode {
		width: Math.min(largeButtonWidth, parent.width, parent.height)
		height: width
		anchors.centerIn: parent
		isForLogin: true
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
