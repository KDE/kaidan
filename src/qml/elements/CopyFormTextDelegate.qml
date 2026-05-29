// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan

/**
 * This is a form text delegate with a button to copy its description.
 */
FormCard.FormTextDelegate {
	id: root
	background: NonInteractiveFormDelegateBackground {}
	trailing: IconButton {
		id: copyButton
		text: qsTr("Copy")
		icon.source: "edit-copy-symbolic"
		onClicked: Utils.copyToClipboard(root.description)
	}
}
