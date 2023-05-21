// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

import "elements"

/**
 * This page is the first page.
 *
 * It is displayed if no account is available.
 */
Kirigami.Page {
	title: "Kaidan"

	ColumnLayout {
		anchors.fill: parent

		Image {
			source: Utils.getResourcePath("images/kaidan.svg")
			Layout.alignment: Qt.AlignCenter
			Layout.fillWidth: true
			Layout.fillHeight: true
			fillMode: Image.PreserveAspectFit
			mipmap: true
			sourceSize.width: width
			sourceSize.height: height
			Layout.bottomMargin: Kirigami.Units.gridUnit * 1.5
		}

		CenteredAdaptiveText {
			text: "Kaidan"
			scaleFactor: 6
		}

		CenteredAdaptiveText {
			text: qsTr("Enjoy free communication on every device!")
			scaleFactor: 2
		}

		// placeholder
		Item {
			Layout.fillHeight: true
			Layout.minimumHeight: root.height * 0.08
		}

		ColumnLayout {
			Layout.alignment: Qt.AlignHCenter
			Layout.maximumWidth: largeButtonWidth

			CenteredAdaptiveHighlightedButton {
				id: startButton
				text: qsTr("Let's start")
				onClicked: pageStack.layers.push(qrCodeOnboardingPage)
			}

			// placeholder
			Item {
				height: startButton.height
			}
		}
	}

	Connections {
		target: Kaidan

		function onConnectionErrorChanged() {
			if (Kaidan.connectionError !== ClientWorker.NoError)
				passiveNotification(Utils.connectionErrorMessage(Kaidan.connectionError))
		}
	}
}
