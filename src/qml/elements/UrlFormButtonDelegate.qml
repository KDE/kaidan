// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan

/**
 * This is a form button delegate for a URL.
 *
 * Its main purpose is to open the URL externally when the button is clicked.
 * It includes a secondary button for copying the URL instead of opening it.
 */
FormCard.FormTextDelegate {
	id: root

	property url url
	property alias copyButton: copyButton

	background: FormCard.FormDelegateBackground {
		control: parent
	}
	trailing: IconButton {
		id: copyButton
		text: qsTr("Copy web address")
		icon.source: "edit-copy-symbolic"
		onClicked: Utils.copyToClipboard(root.url)
	}
	onClicked: Qt.openUrlExternally(url)
}
