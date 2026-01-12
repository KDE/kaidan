// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import org.kde.kirigamiaddons.formcard as FormCard

/**
 * This is a background with a static color for a form delegate.
 */
FormCard.FormDelegateBackground {
	control: parent
	color: secondaryBackgroundColor
	opacity: reducedBackgroundOpacity
}
