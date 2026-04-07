// SPDX-FileCopyrightText: 2026 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami
import org.freedesktop.gstreamer.Qt6GLVideoItem

import im.kaidan.kaidan

import "elements"

/**
 * This page is used for audio/video calls.
 */
Kirigami.Page {
	id: root
	title: qsTr("Call")
	padding: 0
	Component.onDestruction: pageStack.currentItem.forceActiveFocus()

	ScalableText {
		text: rosterItemWatcher.item.displayName
		scaleFactor: avatar.width * 0.02
		elide: Text.ElideMiddle
		width: parent.width - Kirigami.Units.largeSpacing * 2
		visible: !contactCameraArea.visible
		horizontalAlignment: Text.AlignHCenter
		anchors {
			horizontalCenter: parent.horizontalCenter
			topMargin: Kirigami.Units.gridUnit
			top: parent.top
		}
	}

	AccountRelatedAvatar {
		id: avatar
		chatAvatar {
			jid: MainController.activeCall && MainController.activeCall.chatJid
			name: rosterItemWatcher.item.displayName
			asynchronous: false
			implicitWidth: Math.min(root.width, root.height) * 0.3
			implicitHeight: chatAvatar.implicitWidth
		}
		accountAvatar {
			jid: MainController.activeCall && MainController.activeCall.accountJid
			name: MainController.activeCall && AccountController.account(MainController.activeCall.accountJid).settings.displayName
			asynchronous: false
		}
		visible: !contactCameraArea.visible
		anchors.centerIn: parent
	}

	GstGLQt6VideoItem {
		id: contactCameraArea
		objectName: "callPageContactCameraArea"
		visible: MainController.activeCall && MainController.activeCall.videoPlaybackActive
		anchors.fill: parent
	}

	MediaButton {
		id: callStopButton
		iconSource: "call-stop-symbolic"
		iconSize: Kirigami.Units.iconSizes.medium
		iconColor: Kirigami.Theme.negativeTextColor
		strongBackgroundOpacityChange: !contactCameraArea.visible
		anchors {
			horizontalCenter: parent.horizontalCenter
			bottom: parent.bottom
			bottomMargin: Kirigami.Units.smallSpacing * 3
		}
		onClicked: {
			MainController.activeCall.hangUp()
			popLayer()
		}
	}

	RosterItemWatcher {
		id: rosterItemWatcher
		accountJid: MainController.activeCall && MainController.activeCall.accountJid
		jid: MainController.activeCall && MainController.activeCall.chatJid
	}

	Connections {
		target: MainController

		function onActiveCallChanged() {
			if (!MainController.activeCall) {
				popLayer()
			}
		}
	}
}
