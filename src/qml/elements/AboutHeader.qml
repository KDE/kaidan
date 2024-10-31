// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

FormInfoHeader {
	id: root

	Image {
		source: Utils.getResourcePath("images/kaidan.svg")
		Layout.alignment: Qt.AlignHCenter
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
