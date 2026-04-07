// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import QtMultimedia as Multimedia
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

MediaPlayerControl {
	id: root

	property var file
	property Item message
	property real minimumWidth
	property real maximumWidth

	mainBackground {
		opacity: 1
		radius: roundedCornersRadius

		Behavior on opacity {
			NumberAnimation {}
		}
	}
	playButton.backgroundColor: Kirigami.Theme.highlightedTextColor
	player.source: root.file.localFileUrl
	playPositionSlider {
		Layout.minimumWidth: root.minimumWidth - root.leftPadding - parent.spacing * 2 - playButton.width - durationText.width - root.rightPadding
		Layout.maximumWidth: root.maximumWidth - root.leftPadding - parent.spacing * 2 - playButton.width - durationText.width - root.rightPadding
	}
	durationText.data: HoverHandler {
		id: durationTextHoverHandler
	}
	rightClickAction: (mouseArea) => {
		root.showContextMenu(mouseArea)
	}

	MediumMouseArea {
		// This mouse area must be below the other elements to not overlap their mouse areas.
		z: -1
		opacityItem: root.mainBackground
		selected: root.message.contextMenu?.file === file || root.playButton.hovered || root.playPositionSlider.hovered || durationTextHoverHandler.hovered
		hoverEnabled: !(root.playButton.hovered || root.playPositionSlider.hovered || durationTextHoverHandler.hovered)
		acceptedButtons: Qt.RightButton
		onClicked: root.showContextMenu(this)
	}

	function showContextMenu(mouseArea) {
		message.showContextMenu(mouseArea, file)
	}
}
