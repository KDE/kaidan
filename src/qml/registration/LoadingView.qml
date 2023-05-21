// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14

/**
 * This view shows a busy indicator.
 *
 * It is displayed during network interaction.
 */
View {
	descriptionText: qsTr("Requesting the server…")

	Controls.BusyIndicator {
		parent: contentArea
		Layout.alignment: Qt.AlignHCenter
		Layout.fillWidth: true
		Layout.fillHeight: true
		Layout.maximumWidth: parent.width * 0.2
		Layout.maximumHeight: Layout.maximumWidth
	}
}
