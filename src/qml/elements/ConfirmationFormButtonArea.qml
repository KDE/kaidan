// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigami 2.19 as Kirigami
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

/**
 * This area contains a button for an action and a button for confirming the action.
 *
 * It is intended to be used within the contentItem of a MobileForm.FormCard.
 */
ColumnLayout {
	property alias button: button
	property alias confirmationButton: confirmationButton
	property alias confirmationText: confirmationButton.idleText
	property alias busyText: confirmationButton.busyText
	property alias busy: confirmationButton.busy

	spacing: 0

	MobileForm.FormButtonDelegate {
		id: button
		onClicked: confirmationButton.visible = !confirmationButton.visible
	}

	BusyIndicatorFormButton {
		id: confirmationButton
		idleText: qsTr("Confirm")
		visible: false
	}
}
