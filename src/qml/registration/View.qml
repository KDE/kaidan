// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

import "../elements"

/**
 * This is the base for views of the swipe view.
 */
ColumnLayout {
	property alias descriptionText: description.text
	property alias contentArea: contentArea
	property string imageSource

	GridLayout {
		id: contentArea
		Layout.alignment: Qt.AlignHCenter
		Layout.maximumWidth: largeButtonWidth
		Layout.margins: 15
		columns: 1
		rowSpacing: root.height * 0.05

		Image {
			id: image
			source: imageSource ? Utils.getResourcePath("images/onboarding/" + imageSource + ".svg") : ""
			visible: imageSource
			Layout.fillWidth: true
			Layout.fillHeight: true
			fillMode: Image.PreserveAspectFit
		}

		CenteredAdaptiveText {
			id: description
			lineHeight: 1.5
		}
	}
}
