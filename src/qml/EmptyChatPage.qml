// SPDX-FileCopyrightText: 2018 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.19 as Kirigami

import "elements"

ChatPageBase {
	RowLayout {
		spacing: 0
		anchors.fill: parent

		Item {
			Layout.fillWidth: true
		}

		ChatInfo {
			text: qsTr("Select a chat to start")
			level: 1
			type: Kirigami.Heading.Type.Primary
			wrapMode: Text.Wrap
			horizontalAlignment: Text.AlignHCenter
			Layout.maximumWidth: parent.width
		}

		Item {
			Layout.fillWidth: true
		}
	}
}
