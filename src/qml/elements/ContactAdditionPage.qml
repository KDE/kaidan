// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import org.kde.kirigami 2.19 as Kirigami

Kirigami.ScrollablePage {
	id: root

	property alias jid: content.jid
	property alias name: content.name

	title: qsTr("Add contact")
	Component.onCompleted: content.jidField.forceActiveFocus()

	ContactAdditionContent {
		id: content
	}
}
