// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami

/**
 * This page is the base for confirmation pages.
 *
 * It provides an adjustable description, a configurable confirmation button and a predefined cancel button.
 */
BinaryDecisionPage {
	topImageSource: "emblem-ok-symbolic"
	bottomImageSource: "dialog-cancel"
	topActionAsMainAction: true

	signal canceled

	bottomAction: Kirigami.Action {
		text: qsTr("Cancel")
		onTriggered: canceled()
	}
}
