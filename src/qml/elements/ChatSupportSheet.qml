// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

Kirigami.OverlaySheet {
	id: root

	required property var chatSupportList
	property bool isGroupChatSupportSheet: false

	header: Kirigami.Heading {
		text: isGroupChatSupportSheet ? qsTr("Support Group") : qsTr("Support")
		wrapMode: Text.WordWrap
	}

	ListView {
		implicitWidth: largeButtonWidth
		clip: true
		model: Array.from(chatSupportList)

		delegate: Kirigami.AbstractListItem {
			required property int index
			required property string modelData
			readonly property string chatName: {
				(isGroupChatSupportSheet ? qsTr("Group Support %1") : qsTr("Support %1")).arg(index + 1)
			}

			height: 65

			ColumnLayout {
				spacing: 12

				Controls.Label {
					text: chatName
					font.bold: true
					Layout.fillWidth: true
				}

				Controls.Label {
					text: modelData
					wrapMode: Text.Wrap
					Layout.fillWidth: true
				}
			}

			onClicked: {
				if (isGroupChatSupportSheet) {
					Qt.openUrlExternally("xmpp:" + modelData + "?join")
				} else {
					let contactAdditionContainer = openView(contactAdditionDialog, contactAdditionPage)
					contactAdditionContainer.jid = modelData
					contactAdditionContainer.name = chatName
				}

				root.close()
			}
		}
	}
}
