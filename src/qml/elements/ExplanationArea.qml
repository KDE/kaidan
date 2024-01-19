// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami

GridLayout {
	property alias primaryExplanationText: primaryExplanationText
	property alias primaryExplanationImage: primaryExplanationImage
	property alias secondaryExplanationText: secondaryExplanationText
	property alias secondaryExplanationImage: secondaryExplanationImage

	flow: parent.width > parent.height ? GridLayout.LeftToRight : GridLayout.TopToBottom
	rowSpacing: Kirigami.Units.largeSpacing * 2
	columnSpacing: rowSpacing
	width: parent.width
	height: parent.height

	ColumnLayout {
		Layout.preferredWidth: parent.flow === GridLayout.TopToBottom ? parent.width : parent.width / 2 - parent.columnSpacing * 2
		Layout.preferredHeight: parent.flow === GridLayout.LeftToRight ? parent.height : parent.height / 2 - parent.rowSpacing * 2

		CenteredAdaptiveText {
			id: primaryExplanationText
			scaleFactor: 1.5
		}

		Image {
			id: primaryExplanationImage
			sourceSize: Qt.size(860, 860)
			fillMode: Image.PreserveAspectFit
			mipmap: true
			Layout.fillHeight: true
			Layout.fillWidth: true
		}
	}

	Kirigami.Separator {
		Layout.fillWidth: parent.flow === GridLayout.TopToBottom
		Layout.fillHeight: !Layout.fillWidth
		Layout.topMargin: parent.flow === GridLayout.LeftToRight ? parent.height * 0.1 : 0
		Layout.bottomMargin: Layout.topMargin
		Layout.leftMargin: parent.flow === GridLayout.TopToBottom ? parent.width * 0.1 : 0
		Layout.rightMargin: Layout.leftMargin
		Layout.alignment: Qt.AlignCenter
	}

	ColumnLayout {
		Layout.preferredWidth: parent.flow === GridLayout.TopToBottom ? parent.width : parent.width / 2 - parent.columnSpacing * 2
		Layout.preferredHeight: parent.flow === GridLayout.LeftToRight ? parent.height : parent.height / 2 - parent.rowSpacing * 2

		CenteredAdaptiveText {
			id: secondaryExplanationText
			scaleFactor: 1.5
		}

		Image {
			id: secondaryExplanationImage
			sourceSize: Qt.size(860, 860)
			fillMode: Image.PreserveAspectFit
			mipmap: true
			Layout.fillHeight: true
			Layout.fillWidth: true
		}
	}
}
