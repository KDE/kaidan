// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import QtQml.Models 2.14
import org.kde.kirigami 2.19 as Kirigami

/**
 * This is used for displaying content that can be expanded and collapsed.
 *
 * The area is intended to be added to a view (e.g., ListView) and thus managed by its model.
 * It is opened on creation by increasing its height and closed via close() by decreasing it again.
 */
Rectangle {
	default property alias __data: contentArea.data
	property ObjectModel model

	width: parent ? parent.width : 0
	height: enabled ? contentArea.height + Kirigami.Units.largeSpacing * 4 : 0
	clip: true
	color: primaryBackgroundColor
	enabled: false
	onHeightChanged: {
		if (height === 0) {
			model.remove(this.ObjectModel.index)
		}
	}
	Component.onCompleted: {
		// Show this view.
		enabled = true
	}

	Behavior on height {
		SmoothedAnimation {
			velocity: 550
		}
	}

	// top: colored separator
	Rectangle {
		height: 3
		color: Kirigami.Theme.highlightColor
		anchors {
			left: parent ? parent.left : undefined
			right: parent ? parent.right : undefined
			top: parent ? parent.top : undefined
		}
	}

	// middle: content
	ColumnLayout {
		id: contentArea
		anchors.centerIn: parent
		anchors.margins: Kirigami.Units.largeSpacing
		width: parent.width - anchors.margins * 2
	}

	// bottom: colored separator
	Rectangle {
		height: 3
		color: Kirigami.Theme.highlightColor
		anchors {
			left: parent ? parent.left : undefined
			right: parent ? parent.right : undefined
			bottom: parent ? parent.bottom : undefined
		}
	}

	function close() {
		enabled = false
	}
}
