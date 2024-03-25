// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2023 Mathis Brüchert <mbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami
import im.kaidan.kaidan 1.0
import org.kde.kirigamiaddons.formcard

/**
 * The settings page contains options to configure Kaidan.
 *
 * It is used on a new layer on mobile and inside of a Sheet on desktop.
 */
ColumnLayout {
	property string title: qsTr("Settings")

	Layout.fillHeight: true

	FormCard {
		Layout.fillWidth: true
		delegates: ColumnLayout {
			spacing: 0

			FormButtonDelegate {
				text: qsTr("Multimedia Settings")
				description: qsTr("Configure photo, video and audio recording settings")
				onClicked: stack.push("qrc:/qml/settings/MultimediaSettings.qml")
				icon.name: "emblem-system-symbolic"
			}

			FormButtonDelegate {
				text: qsTr("About Kaidan")
				description: qsTr("Learn about the current Kaidan version, view the source code and contribute")
				onClicked: stack.push("qrc:/qml/settings/AboutPage.qml")
				icon.name: "help-about-symbolic"
			}
		}
	}
}
