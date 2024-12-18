// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/**
 * This page is the base for confirmation pages.
 *
 * It provides an adjustable description, a configurable confirmation button and a predefined cancel button.
 */
BinaryDecisionPage {
	topImageSource: "emblem-ok-symbolic"
	bottomImageSource: "window-close-symbolic"
	topActionAsMainAction: true

	signal canceled

	bottomAction: Kirigami.Action {
		text: qsTr("Cancel")
		onTriggered: canceled()
	}
}
