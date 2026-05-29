// SPDX-FileCopyrightText: 2026 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami

/**
 * Icon-only button.
 */
Button {
	id: root

	property alias iconFallback: contentIcon.fallback

	icon.width: Kirigami.Units.iconSizes.small
	contentItem: Kirigami.Icon {
		id: contentIcon
		source: root.icon.source
		color: root.icon.color
		isMask: !Qt.colorEqual(color, "transparent")
		implicitWidth: root.icon.width
		implicitHeight: root.icon.height
	}
	horizontalPadding: Math.round(icon.width * 0.5)
	verticalPadding: Math.round(icon.height * 0.5)
	topInset: 0
	bottomInset: 0
	leftInset: 0
	rightInset: 0
	Component.onCompleted: icon.height = icon.width
}
