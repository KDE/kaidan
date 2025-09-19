// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan

/**
 * This is a dialog to select an account before proceeding with an account-related action.
 */
Dialog {
	id: root

	property bool groupChatCreationSupported: false
	property bool groupChatParticipationSupported: false
	signal accountSelected(Account account)

	topPadding: 0
	leftPadding: 0
	bottomPadding: 0
	rightPadding: 0
	header: null

	ListView {
		implicitHeight: contentHeight
		model: AccountController.enabledAccounts()
		delegate: FormCard.FormButtonDelegate {
			leading: Avatar {
				jid: modelData.settings.jid
				name: modelData.settings.displayName
				opacity: enabled ? 1 : 0.5
			}
			leadingPadding: 10
			text: modelData.settings.displayName
			description: qsTr("Feature unsupported")
			descriptionItem.font.italic: true
			descriptionItem.visible: !enabled
			width: ListView.view.width
			enabled: (!root.groupChatCreationSupported || modelData.groupChatController.groupChatCreationSupported) && (!root.groupChatParticipationSupported || modelData.groupChatController.groupChatParticipationSupported)
			onClicked: {
				root.accountSelected(modelData)
				root.close()
			}
		}
	}
}
