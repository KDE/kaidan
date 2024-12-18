// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami

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
