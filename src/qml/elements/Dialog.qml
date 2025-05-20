// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

/**
 * This is a base for a dialog.
 */
Kirigami.Dialog {
	footer: null
	preferredWidth: largeButtonWidth
	maximumWidth: preferredWidth
	maximumHeight: applicationWindow().height - Kirigami.Units.gridUnit * 6
	// Set negative insets to fix an overlapping scrollbar and preserve the rounded corners of the dialog.
	topInset: contentItem.Controls.ScrollBar.vertical.visible  && !header ? - Kirigami.Units.cornerRadius : 0
	bottomInset: contentItem.Controls.ScrollBar.vertical.visible ? - Kirigami.Units.cornerRadius : 0
	rightInset: contentItem.Controls.ScrollBar.vertical.visible ? - __borderWidth : 0
	onClosed: destroy()
}
