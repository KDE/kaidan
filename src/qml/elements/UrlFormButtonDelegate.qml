// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

import im.kaidan.kaidan 1.0

/**
 * This is a form button delegate for a URL.
 *
 * Its main purpose is to open the URL externally when the button is clicked.
 * It includes a secondary button for copying the URL instead of opening it.
 */
MobileForm.FormTextDelegate {
	required property string url

	background: MobileForm.FormDelegateBackground { control: parent }
	trailing: Button {
		icon.name: "edit-copy-symbolic"
		onClicked: Utils.copyToClipboard(url)
	}
	onClicked: Qt.openUrlExternally(url)
}
