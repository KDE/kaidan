// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls
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
	property string confirmationText: qsTr("Confirm")
	property string busyText
	property alias busy: busyIndicator.visible

	MobileForm.FormButtonDelegate {
		id: button
		onClicked: confirmationButton.visible = !confirmationButton.visible
	}

	MobileForm.FormButtonDelegate {
		id: confirmationButton
		text: busyIndicator.visible ? busyText : confirmationText
		visible: false
		font.italic: busyIndicator.visible
		leading: Controls.BusyIndicator {
			id: busyIndicator
			visible: false
			implicitWidth: Kirigami.Units.iconSizes.smallMedium
			implicitHeight: Kirigami.Units.iconSizes.smallMedium
		}
		// The paddings are needed to position busyIndicator correctly.
		// It would otherwise have a too large left margin.
		leadingPadding: Kirigami.Units.largeSpacing
		leftPadding: Kirigami.Units.largeSpacing * 2
		onClicked: busyIndicator.visible = true
	}
}
