// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/**
 * This button contains a busy indicator for an action without an instantaneous result.
 *
 * It is intended to be used within the contentItem of a FormCard.FormCard.
 */	
FormCard.FormButtonDelegate {
	property string idleText
	property string busyText
	property bool busy: false
	property string idleIconSource: "emblem-ok-symbolic"

	text: busy ? busyText : idleText
	enabled: !busy
	font.italic: busy
	// Needed to position "leading" appropriately.
	// It would otherwise have a too small right margin.
	leadingPadding: Kirigami.Units.smallSpacing * 3
	// Using `"icon.name: busy ? "" : "emblem-ok-symbolic"` and setting busyIndicator as leading
	// results in a strange larger leftPadding for the icon if busyIndicator is not loaded.
	// Thus, the icon is set as leading as well.
	leading: Loader {
		sourceComponent: busy ? busyIndicator : idleIcon

		Component {
			id: idleIcon

			Kirigami.Icon {
				source: idleIconSource
				implicitWidth: Kirigami.Units.iconSizes.small
				implicitHeight: Kirigami.Units.iconSizes.small
			}
		}

		Component {
			id: busyIndicator

			Controls.BusyIndicator {
				implicitWidth: Kirigami.Units.iconSizes.small
				implicitHeight: implicitWidth
				// Needed to make the item larger without increasing the parent's height.
				padding: - 2
			}
		}
	}
}
