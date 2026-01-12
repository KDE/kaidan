// SPDX-FileCopyrightText: 2026 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import org.kde.kirigamiaddons.formcard as FormCard

/**
 * Button used as a primary item to expand and collapse secondary form entries below it.
 */
FormCard.FormButtonDelegate {
	checkable: true
	trailingLogo.direction: checked ? Qt.DownArrow : Qt.RightArrow
}
