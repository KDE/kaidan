// SPDX-FileCopyrightText: 2018 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2018 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * This element is used in the @see SendMediaSheet to display information about a selected video to
 * the user. It just displays the video in a rectangle.
 */

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtMultimedia 5.14 as Multimedia
import org.kde.kirigami 2.19 as Kirigami

MediaPreviewAudio {
	id: root

	Layout.preferredHeight: message ? messageSize : Kirigami.Units.gridUnit * 18
	Layout.preferredWidth: Kirigami.Units.gridUnit * 32
	Layout.maximumWidth: message ? messageSize : -1

	placeHolder: Multimedia.VideoOutput {
		source: root.player
		fillMode: Image.PreserveAspectFit

		anchors {
			fill: parent
		}

		Kirigami.Icon {
			source: "video-x-generic"
			visible: root.player.playbackState === Multimedia.MediaPlayer.StoppedState

			width: parent.width
			height: parent.height

			anchors {
				centerIn: parent
			}
		}

		MouseArea {
			anchors {
				fill: parent
			}

			onPressed: root.playPauseButton.clicked()
		}
	}
}
