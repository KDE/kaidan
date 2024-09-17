// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigami 2.19 as Kirigami
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

import im.kaidan.kaidan 1.0

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
