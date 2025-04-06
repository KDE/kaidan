// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/**
 * This is a base for a dialog.
 */
Kirigami.Dialog {
	footer: null
	preferredWidth: largeButtonWidth
	maximumWidth: preferredWidth
	maximumHeight: applicationWindow().height - Kirigami.Units.gridUnit * 6
	onClosed: destroy()
}
