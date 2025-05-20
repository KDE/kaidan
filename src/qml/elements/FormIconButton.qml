// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/**
 * Form button with a centered icon.
 */
FormCard.AbstractFormDelegate {
	id: root
	icon {
		width: Kirigami.Units.iconSizes.small
		height: Kirigami.Units.iconSizes.small
	}
	contentItem: RowLayout {
		Item {
			Layout.fillWidth: true
		}

		Kirigami.Icon {
			source: root.icon.name
			implicitWidth: root.icon.width
			implicitHeight: root.icon.height
		}

		Item {
			Layout.fillWidth: true
		}
	}
	leftPadding: 0
	rightPadding: 0
	Layout.fillWidth: true
}
