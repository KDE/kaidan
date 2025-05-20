// SPDX-FileCopyrightText: 2020 Mathis Brüchert <mbblp@protonmail.ch>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2023 Bhavy Airi <airiraghav@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan

/**
 * This page is used for authenticating encryption keys of the own account by scanning QR codes or entering key IDs.
 */
KeyAuthenticationPage {
	id: root
	jid: account.settings.jid
	explanation: ExplanationArea {
		primaryExplanationText.text: qsTr("Step 1: Scan your <b>other device's</b> QR code")
		primaryExplanationImage.source: Utils.getResourcePath("images/qr-code-scan-own-1.svg")
		secondaryExplanationText.text: qsTr("Step 2: Scan with your other device <b>this device's</b> QR code")
		secondaryExplanationImage.source: Utils.getResourcePath("images/qr-code-scan-own-2.svg")
	}
	authenticatableKeysArea.header.title: qsTr("Unverified own devices")
	authenticatableKeysArea.listView.model: AuthenticatableEncryptionKeyModel {
		encryptionController: root.account.encryptionController
		accountJid: root.jid
		chatJid: root.jid
	}
}
