// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick

/**
 * This is a list view that can be placed inside of a flickable element.
 *
 * That way, the list view is not moved out of the parent element when the list view is flicked.
 */
ListView {
	interactive: false
	clip: true
}
