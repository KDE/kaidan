// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/**
 * Used to expand and collapse form entries.
 */
FormCard.AbstractFormDelegate {
	id: root
	leftPadding: 0
	rightPadding: 0
	checkable: true
	contentItem: RowLayout {
		Item {
			Layout.fillWidth: true
		}

		Kirigami.Icon {
			source: root.checked ? "go-up-symbolic" : "go-down-symbolic"
			implicitWidth: Kirigami.Units.iconSizes.small
			implicitHeight: implicitWidth
		}

		Item {
			Layout.fillWidth: true
		}
	}
	Layout.fillWidth: true
}
