// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigamiaddons.formcard as FormCard

/**
 * This area is used to interact with encryption keys.
 */
GridLayout {
	property alias header: header
	property alias listView: listView

	Layout.maximumHeight: parent.flow === GridLayout.LeftToRight ? parent.height : parent.height / 2 - parent.rowSpacing * 2

	FormCard.FormCard {
		Layout.fillWidth: true
		Layout.maximumHeight: parent.height
		Layout.alignment: Qt.AlignCenter

		FormCard.FormHeader {
			id: header
		}

		Controls.ScrollView {
			Layout.fillWidth: true
			Layout.fillHeight: true
			Layout.preferredWidth: contentWidth
			Layout.preferredHeight: contentHeight
			clip: true

			ListView {
				id: listView
			}
		}
	}
}
