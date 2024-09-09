// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

import im.kaidan.kaidan 1.0

/**
 * This is a form button delegate for a URL.
 *
 * Its main purpose is to open the URL externally when the button is clicked.
 * It includes a secondary button for copying the URL instead of opening it.
 */
MobileForm.FormTextDelegate {
	property url url
	property alias copyButton: copyButton

	background: MobileForm.FormDelegateBackground { control: parent }
	trailing: Button {
		id: copyButton
		Controls.ToolTip.text: qsTr("Copy web address")
		icon.name: "edit-copy-symbolic"
		onClicked: Utils.copyToClipboard(url)
	}
	onClicked: Qt.openUrlExternally(url)
}
