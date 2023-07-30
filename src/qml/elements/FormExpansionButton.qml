// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls
import org.kde.kirigami 2.19 as Kirigami
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

/**
 * Used to expand and collapse form entries.
 */
MobileForm.AbstractFormDelegate {
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
