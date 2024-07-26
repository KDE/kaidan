// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import QtQuick.Layouts 1.15
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

	Controls.Label {
		text: qsTr("Messages in this chat are synchronized as notes across all your devices")
		visible: accountJid === jid
		color: Kirigami.Theme.neutralTextColor
		wrapMode: Text.WordWrap
		Layout.fillWidth: true
		Layout.leftMargin: Kirigami.Units.largeSpacing * 5
	}

	RosterItemWatcher {
		id: contactWatcher
		jid: root.jid
	}

	function changeDisplayName(newDisplayName) {
		Kaidan.client.rosterManager.renameContactRequested(root.jid, newDisplayName)
	}

	function handleDisplayNameChanged() {}
}
