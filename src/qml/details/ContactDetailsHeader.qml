// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

import "../elements"

DetailsHeader {
	id: root
	displayName: contactWatcher.item.displayName
	avatarAction: Kirigami.Action {
		text: qsTr("Maximize avatar")
		icon.name: "view-fullscreen-symbolic"
		enabled: Kaidan.avatarStorage.getAvatarUrl(jid)
		onTriggered: Qt.openUrlExternally(Kaidan.avatarStorage.getAvatarUrl(jid))
	}

	property bool isChatWithOneself: MessageModel.currentAccountJid === jid

	RosterItemWatcher {
		id: contactWatcher
		jid: root.jid
	}

	RowLayout {
		visible: !isChatWithOneself
		spacing: Kirigami.Units.smallSpacing

		UserPresenceWatcher {
			id: userPresence
			jid: root.jid
		}

		Kirigami.Icon {
			id: availabilityIcon
			source: userPresence.availabilityIcon
			visible: contactWatcher.item.sendingPresence
			Layout.preferredWidth: 22
			Layout.preferredHeight: Layout.preferredWidth
			Layout.leftMargin: 5
		}

		// placeholder when availabilityIcon is not visible
		Item {
			visible: !availabilityIcon.visible
			Layout.preferredWidth: availabilityIcon.Layout.preferredWidth
			Layout.preferredHeight: availabilityIcon.Layout.preferredHeight
			Layout.leftMargin: availabilityIcon.Layout.leftMargin
		}

		Controls.Label {
			Layout.alignment: Qt.AlignVCenter
			text: contactWatcher.item.sendingPresence ? userPresence.availabilityText : qsTr("Contact sends no status")
			color: userPresence.availabilityColor
			textFormat: Text.PlainText
			elide: Text.ElideRight
			Layout.fillWidth: true
			Layout.leftMargin: 9
		}

		Item {
			Layout.fillWidth: true
		}
	}

	function displayNameChangeFunction(newDisplayName) {
		Kaidan.client.rosterManager.renameContactRequested(root.jid, newDisplayName)
	}

	function displayNameChangedFunction() {}
}
