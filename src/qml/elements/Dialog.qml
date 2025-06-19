// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

/**
 * This is a base for a dialog.
 */
Kirigami.Dialog {
	footer: null
	// Set a negative inset to fix the rounded corner of the dialog below the scroll bar if visible.
	bottomInset: contentItem.Controls.ScrollBar.vertical.visible ? - Kirigami.Units.cornerRadius : 0
	preferredWidth: largeButtonWidth
	maximumWidth: preferredWidth
	maximumHeight: applicationWindow().height - Kirigami.Units.gridUnit * 6
	onClosed: destroy()
}
