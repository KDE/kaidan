// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14

/**
 * This is a rectangle with rounded corners having a radius relative to the rectangle's dimensions.
 *
 * It should be used to maintain a consistent look of the corners fitting the rectangle's size.
 */
Rectangle {
	radius: relativeRoundedCornersRadius(width, height)
}
