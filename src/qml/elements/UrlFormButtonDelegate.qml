// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
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
	trailing: Button {
		id: copyButton
		Controls.ToolTip.text: qsTr("Copy web address")
		icon.name: "edit-copy-symbolic"
		onClicked: Utils.copyToClipboard(root.url)
	}
	onClicked: Qt.openUrlExternally(url)
}
