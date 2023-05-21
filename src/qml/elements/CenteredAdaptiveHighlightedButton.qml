// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

/**
 * This is a highlighted button.
 *
 * It is used for main actions.
 */
CenteredAdaptiveButton {
	Kirigami.Theme.textColor: Style.buttonColoringEnabled ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
	Kirigami.Theme.backgroundColor: positive ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.negativeTextColor

	// Set the property to 'false' for a cancellation or a dangerous action.
	property bool positive: true
}
