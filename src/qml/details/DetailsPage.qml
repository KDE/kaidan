// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

Kirigami.ScrollablePage {
	id: root

	required property string jid
	required property Component mainComponent

	leftPadding: 0
	rightPadding: 0
	Kirigami.Theme.colorSet: Kirigami.Theme.Window

	DetailsContentContainer {
		jid: root.jid
		qrCodeButton: header.qrCodeButton
		mainComponent: root.mainComponent
	}
}
