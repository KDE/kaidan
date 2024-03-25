// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls
import org.kde.kirigamiaddons.formcard

/**
 * This area is used to interact with encryption devices and their keys.
 */
GridLayout {
    // TODO property alias header: header
    // TODO property alias listView: listView

	Layout.maximumHeight: parent.flow === GridLayout.LeftToRight ? parent.height : parent.height / 2 - parent.rowSpacing * 2

    FormCard {
		Layout.fillWidth: true
		Layout.maximumHeight: parent.height
		Layout.alignment: Qt.AlignCenter

        delegates: ColumnLayout {
			spacing: 0

            FormHeader {
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
}
