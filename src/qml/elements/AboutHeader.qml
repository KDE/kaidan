// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

FormInfoHeader {
	id: root

	Image {
		source: Utils.getResourcePath("images/kaidan.svg")
		Layout.alignment: Qt.AlignHCenter
		Layout.margins: - Kirigami.Units.largeSpacing
		Layout.preferredHeight: Kirigami.Units.gridUnit * 8
		Layout.preferredWidth: Layout.preferredHeight
		fillMode: Image.PreserveAspectFit
		mipmap: true
		sourceSize: Qt.size(width, height)
	}

	ColumnLayout {
		spacing: Kirigami.Units.largeSpacing
		Layout.leftMargin: root.flow === GridLayout.LeftToRight ? Kirigami.Units.largeSpacing * 2 : 0

		Kirigami.Heading {
			id: applicationNameHeading
			text: Utils.applicationDisplayName + " " + Utils.versionString
			textFormat: Text.PlainText
			wrapMode: Text.WordWrap
			Layout.fillWidth: true
			horizontalAlignment: Qt.AlignLeft
		}

		Controls.Label {
			text: qsTr("Modern chat app for every device")
			font.italic: true
			wrapMode: Text.WordWrap
			Layout.fillWidth: true
			horizontalAlignment: Qt.AlignLeft
		}
	}
}
