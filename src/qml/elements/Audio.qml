// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import QtMultimedia as Multimedia
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

ExtendedMessageContent {
	id: root

	property var file
	property Item message

	mainArea.data: MediumMouseArea {
		// This mouse area must be below the other elements to not overlap their mouse areas.
		z: - 1
		opacityItem: parent.background
		selected: (root.message.contextMenu && root.message.contextMenu.file === file) || playButton.active || playPositionSlider.hovered
		acceptedButtons: Qt.RightButton
		anchors.fill: parent
		onClicked: root.showContextMenu(this)
	}
	mainAreaBackground {
		Behavior on opacity {
			NumberAnimation {}
		}
	}
	contentArea.data: [
		ClickableIcon {
			id: playButton

			readonly property bool playing: player.playbackState === Multimedia.MediaPlayer.PlayingState

			Controls.ToolTip.text: playing ? qsTr("Pause") : qsTr("Play")
			source: playing ? "media-playback-pause-symbolic" : "media-playback-start-symbolic"
			Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
			onClicked: playing ? player.pause() : player.play()
		},

		Slider {
			id: playPositionSlider
			to: player.duration
			value: player.position
			Layout.preferredWidth: Kirigami.Units.gridUnit * 15
			Layout.minimumWidth: root.minimumWidth - mainArea.leftPadding - parent.spacing * 2 - playButton.width - durationText.width - mainArea.rightPadding
			Layout.maximumWidth: root.maximumWidth - mainArea.leftPadding - parent.spacing * 2 - playButton.width - durationText.width - mainArea.rightPadding
			Layout.fillWidth: true
			onMoved: player.position = value

			// Prevent moving the handle by right clicks.
			// Instead, show the message's context menu.
			MouseArea {
				acceptedButtons: Qt.RightButton
				anchors.fill: parent
				onClicked: root.showContextMenu(this)
			}
		},

		ScalableText {
			id: durationText
			text: MediaUtils.prettyDuration(player.position, player.duration) + "/" + MediaUtils.prettyDuration(player.duration)
			scaleFactor: 0.9
			color: Kirigami.Theme.disabledTextColor
			// A custom padding is used because "verticalAlignment: Text.AlignVCenter"
			// does not work correctly if the text is scaled.
			bottomPadding: font.pixelSize * 0.1
			Layout.minimumWidth: contentWidth
		},

		Multimedia.MediaPlayer {
			id: player
			source: root.file.localFileUrl
			audioOutput: Multimedia.AudioOutput {}
			onPlaybackStateChanged: {
				if (playbackState === Multimedia.MediaPlayer.StoppedState) {
					position = 0
				}
			}
		}
	]

	function showContextMenu(mouseArea) {
		message.showContextMenu(mouseArea, file)
	}
}
