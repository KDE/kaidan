// SPDX-FileCopyrightText: 2018 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2023 Mathis Brüchert <mbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami

/**
 * This sheet is used on desktop systems instead of a new layer. It doesn't
 * fill the complete width, so it looks a bit nicer on large screens.
 */
Kirigami.OverlaySheet {
	id: settingsSheet
	Kirigami.Theme.inherit: false
	Kirigami.Theme.colorSet: Kirigami.Theme.Window
	leftPadding: 0
	rightPadding: 0
	header: RowLayout {
		anchors.fill: parent
		spacing: 1
		Controls.ToolButton {
			id: backButton
			enabled: stack.currentItem !== stack.initialItem
			icon.name: "draw-arrow-back"
			onClicked: stack.pop()
		}
		Kirigami.Heading {
			Layout.fillWidth: true
			Layout.alignment: Qt.AlignVCenter
			text: stack.currentItem.title
		}
	}

	contentItem: Controls.StackView {
		Layout.fillHeight: true
		Layout.fillWidth: true

		id: stack
		implicitHeight: currentItem.implicitHeight
		implicitWidth: 600

		initialItem: SettingsContent {}
		clip: true
	}
}
