// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2018 Ilya Bizyaev <bizyaev@zoho.com>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

import "elements"

Controls.Dialog {
	id: aboutDialog
	modal: true
	standardButtons: Controls.Dialog.Ok
	onAccepted: close()

	GridLayout {
		anchors.fill: parent
		flow: applicationWindow().wideScreen ? GridLayout.LeftToRight : GridLayout.TopToBottom
		columnSpacing: 20
		rowSpacing: 20

		Image {
			source: Utils.getResourcePath("images/kaidan.svg")
			Layout.preferredWidth: Kirigami.Units.gridUnit * 9
			Layout.preferredHeight: Kirigami.Units.gridUnit * 9
			Layout.fillWidth: true
			Layout.fillHeight: true
			Layout.alignment: Qt.AlignCenter
			fillMode: Image.PreserveAspectFit
			mipmap: true
			sourceSize: Qt.size(width, height)
		}

		ColumnLayout {
			Layout.fillWidth: true
			Layout.fillHeight: true
			spacing: Kirigami.gridUnit * 0.6

			Kirigami.Heading {
				text: Utils.applicationDisplayName() + " " + Utils.versionString()
				textFormat: Text.PlainText
				wrapMode: Text.WordWrap
				Layout.fillWidth: true
				horizontalAlignment: Qt.AlignHCenter
			}

			Controls.Label {
				text: "<i>" + qsTr("A simple, user-friendly Jabber/XMPP client") + "</i>"
				textFormat: Text.StyledText
				wrapMode: Text.WordWrap
				Layout.fillWidth: true
				horizontalAlignment: Qt.AlignHCenter
			}

			Controls.Label {
				text: "<b>" + qsTr("License:") + "</b> GPLv3+ / CC BY-SA 4.0"
				textFormat: Text.StyledText
				wrapMode: Text.WordWrap
				Layout.fillWidth: true
				horizontalAlignment: Qt.AlignHCenter
			}

			Controls.Label {
				text: "Copyright © 2016-2023\nKaidan developers and contributors"
				textFormat: Text.PlainText
				wrapMode: Text.WordWrap
				Layout.fillWidth: true
				Layout.preferredWidth: contentWidth
				horizontalAlignment: Qt.AlignHCenter
			}

			CenteredAdaptiveButton {
				text: qsTr("Report problems")
				onClicked: Qt.openUrlExternally(Utils.issueTrackingUrl)
			}

			CenteredAdaptiveButton {
				text: qsTr("View source code online")
				onClicked: Qt.openUrlExternally(Utils.applicationSourceCodeUrl())
			}
		}
	}
}
