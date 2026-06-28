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
 * Additional form entries can be added as children.
 * They are only shown while the action's button is checked (i.e., the content and the confirmation button are expanded).
 *
 * It is intended to be used within a FormCard.FormCard.
 */
ColumnLayout {
	id: root

	default property alias __data: contentArea.data
	property alias button: button
	property alias confirmationButton: confirmationButton
	property alias confirmationText: confirmationButton.idleText
	property alias busyText: confirmationButton.busyText
	property alias busy: confirmationButton.busy
	readonly property bool _bottomCornersRounded: {
		const isLast = parent.children[parent.children.length - 1] === this
		return parent._roundCorners && isLast
	}

	spacing: 0

	FormExpansionButtonDelegate {
		id: button
		background: FormCard.FormDelegateBackground {
			control: parent
			// Needed since the corners would otherwise not be rounded because this button is not a
			// direct child of FormCard.FormCard.
			corners {
				bottomLeftRadius: root._bottomCornersRounded && !confirmationButton.visible ? Kirigami.Units.smallSpacing : 0
				bottomRightRadius: root._bottomCornersRounded && !confirmationButton.visible ? Kirigami.Units.smallSpacing : 0
			}
		}
	}

	ColumnLayout {
		id: contentArea
		spacing: 0
		visible: root.button.checked
		Layout.fillWidth: true
	}

	BusyIndicatorFormButton {
		id: confirmationButton
		idleText: qsTr("Confirm")
		visible: root.button.checked
		background: SecondaryFormButtonBackground {
			control: parent
			// Needed since the corners would otherwise not be rounded because this button is not a
			// direct child of FormCard.FormCard.
			corners {
				bottomLeftRadius: root._bottomCornersRounded ? Kirigami.Units.smallSpacing : 0
				bottomRightRadius: root._bottomCornersRounded ? Kirigami.Units.smallSpacing : 0
			}
		}
	}
}
