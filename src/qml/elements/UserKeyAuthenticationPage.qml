// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan

/**
 * This page is used for authenticating encryption keys of users by scanning QR codes or entering key IDs.
 */
KeyAuthenticationPage {
	id: root
	authenticatableKeysArea.listView.model: AuthenticatableEncryptionKeyModel {
		accountJid: root.accountJid
		chatJid: root.jid
	}
}
