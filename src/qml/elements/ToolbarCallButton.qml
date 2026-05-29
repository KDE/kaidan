// SPDX-FileCopyrightText: 2026 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

/**
 * This is a button to be used for calls inside of the toolbar.
 */
IconButton  {
	icon.color: MainController.activeCall ? Kirigami.Theme.positiveTextColor : "transparent"
}
