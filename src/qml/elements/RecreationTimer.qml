// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/**
 * This timer can be used to recreate an object of a component.
 *
 * E.g., a camera object can be reloaded in case the camera device is not available at the time of
 * creating the camera object.
 * Reloading is needed if the camera device is not plugged in or disabled.
 * That approach ensures that a camera device is even detected after plugging it out and in again.
 */
Timer {
	property Component objectComponent
	property var object: null

	interval: Kirigami.Units.longDuration
	onTriggered: {
		object.destroy()
		createNewObject()
	}
	Component.onCompleted: createNewObject()

	function createNewObject() {
		object = objectComponent.createObject()
	}
}
