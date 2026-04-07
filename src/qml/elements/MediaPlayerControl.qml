// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import QtMultimedia as Multimedia
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

MediaButtonArea {
	id: root

	property alias playButton: playButton
	property alias playPositionSlider: playPositionSlider
	property alias durationText: durationText
	property var rightClickAction
	property Multimedia.MediaPlayer player: Multimedia.MediaPlayer {
		audioOutput: Multimedia.AudioOutput {}
		onPlaybackStateChanged: {
			if (playbackState === Multimedia.MediaPlayer.StoppedState) {
				position = 0
			}
		}
	}

	contentItem: RowLayout {
		MediaButton {
			id: playButton

			readonly property bool playing: player.playbackState === Multimedia.MediaPlayer.PlayingState

			Controls.ToolTip.text: playing ? qsTr("Pause") : qsTr("Play")
			iconSource: playing ? "media-playback-pause-symbolic" : "media-playback-start-symbolic"
			iconSize: Kirigami.Units.iconSizes.small
			strongBackgroundOpacityChange: true
			onClicked: playing ? player.pause() : player.play()
		}

		Slider {
			id: playPositionSlider
			to: player.duration
			value: player.position
			Layout.preferredWidth: Kirigami.Units.gridUnit * 15
			Layout.fillWidth: true
			onMoved: player.position = value

			// Prevent moving the handle by right clicks.
			// Instead, call rightClickAction.
			MouseArea {
				acceptedButtons: Qt.RightButton
				anchors.fill: parent
				onClicked: {
					if (root.rightClickAction) {
						root.rightClickAction(this)
					}
				}
			}
		}

		ScalableText {
			id: durationText
			text: MediaUtils.prettyDuration(player.position, player.duration) + "/" + MediaUtils.prettyDuration(player.duration)
			scaleFactor: 0.9
			color: Kirigami.Theme.disabledTextColor
			rightPadding: Kirigami.Units.mediumSpacing
			// A custom padding is used because "verticalAlignment: Text.AlignVCenter"
			// does not work correctly if the text is scaled.
			bottomPadding: font.pixelSize * 0.1
			Layout.minimumWidth: contentWidth
		}
	}
}
