// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * This element is used in the @see SendMediaSheet to display information about a selected audio file to
 * the user. It just displays the audio in a rectangle.
 */

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls
import QtMultimedia 5.14 as Multimedia
import org.kde.kirigami 2.19 as Kirigami

import MediaUtils 0.1

MediaPreview {
	id: root

	property alias placeHolder: placeHolder.data

	readonly property alias player: mediaPlayer
	readonly property alias playPauseButton: playPause

	color: 'transparent'

	Layout.preferredHeight: message ? Kirigami.Units.gridUnit * 2.4 : Kirigami.Units.gridUnit * 2.45
	Layout.preferredWidth: message ? Kirigami.Units.gridUnit * 10 : Kirigami.Units.gridUnit * 20
	Layout.maximumWidth: message ? messageSize : -1

	Multimedia.MediaPlayer {
		id: mediaPlayer

		source: root.mediaSource
		volume: volumePlayer.volume

		onStopped: seek(0)
	}

	ColumnLayout {
		anchors {
			fill: parent
		}

		Item {
			id: placeHolder

			visible: children.length > 0

			Layout.fillHeight: true
			Layout.fillWidth: true
		}

		RowLayout {
			Layout.topMargin: 6
			Layout.margins: 10
			Layout.fillWidth: true

			Controls.ToolButton {
				id: playPause

				icon.name: mediaPlayer.playbackState === Multimedia.MediaPlayer.PlayingState
						   ? 'media-playback-pause-symbolic'
						   : 'media-playback-start-symbolic'

				onClicked: {
					switch (mediaPlayer.playbackState) {
					case Multimedia.MediaPlayer.PlayingState:
						mediaPlayer.pause()
						break
					case Multimedia.MediaPlayer.PausedState:
					case Multimedia.MediaPlayer.StoppedState:
						mediaPlayer.play()
						break
					}
				}
			}

			Controls.Slider {
				from: 0
				to: mediaPlayer.duration
				value: mediaPlayer.position
				enabled: mediaPlayer.seekable

				Layout.fillWidth: true

				Row {
					anchors {
						right: parent.right
						top: parent.top
						topMargin: -(parent.height / 2)
					}

					readonly property real fontSize: 7

					Controls.Label {
						text: MediaUtilsInstance.prettyDuration(mediaPlayer.position, mediaPlayer.duration)
						font.pointSize: parent.fontSize
						visible: mediaPlayer.duration > 0 && mediaPlayer.playbackState !== Multimedia.MediaPlayer.StoppedState
					}
					Controls.Label {
						text: ' / '
						font.pointSize: parent.fontSize
						visible: mediaPlayer.duration > 0 && mediaPlayer.playbackState !== Multimedia.MediaPlayer.StoppedState
					}
					Controls.Label {
						text: MediaUtilsInstance.prettyDuration(mediaPlayer.duration)
						visible: mediaPlayer.duration > 0
						font.pointSize: parent.fontSize
					}
				}

				onMoved: mediaPlayer.seek(value)
			}

			Controls.Slider {
				id: volumePlayer

				readonly property real volume: Multimedia.QtMultimedia.convertVolume(value,
																					 Multimedia.QtMultimedia.LogarithmicVolumeScale,
																					 Multimedia.QtMultimedia.LinearVolumeScale)

				from: 0
				to: 1.0
				value: 1.0
				visible: !root.message && !Kirigami.Settings.isMobile

				Layout.preferredWidth: root.width / 6
			}

			Controls.ToolButton {
				icon {
					name: 'document-open-symbolic'
				}

				onClicked: Qt.openUrlExternally(root.mediaSource)
			}
		}
	}
}

