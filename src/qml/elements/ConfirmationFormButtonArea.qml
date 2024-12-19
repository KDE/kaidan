// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/**
 * This area contains a button for an action and a button for confirming the action.
 *
 * It is intended to be used within the contentItem of a FormCard.FormCard.
 */
ColumnLayout {
	property alias button: button
	property alias confirmationButton: confirmationButton
	property alias confirmationText: confirmationButton.idleText
	property alias busyText: confirmationButton.busyText
	property alias busy: confirmationButton.busy

	spacing: 0

	FormCard.FormButtonDelegate {
		id: button
		onClicked: confirmationButton.visible = !confirmationButton.visible
	}

	BusyIndicatorFormButton {
		id: confirmationButton
		idleText: qsTr("Confirm")
		visible: false
	}
}
