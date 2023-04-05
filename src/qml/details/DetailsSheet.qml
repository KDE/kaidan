// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

import "../elements"

Kirigami.OverlaySheet {
	id: root

	required property string jid
	required property Component mainComponent

	bottomPadding: 0
	Kirigami.Theme.colorSet: Kirigami.Theme.Window

	contentItem: DetailsContentContainer {
		jid: root.jid
		qrCodeButton: header.qrCodeButton
		mainComponent: root.mainComponent
		Layout.preferredWidth: 600
		Layout.preferredHeight: 600
		Layout.maximumWidth: 600
	}
}
