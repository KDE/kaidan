// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.19 as Kirigami

/**
 * This is a base for a dialog.
 */
Kirigami.Dialog {
	id: root
	footer: Item {}
	preferredWidth: Kirigami.Units.gridUnit * 30
	maximumHeight: applicationWindow().height - Kirigami.Units.gridUnit * 6
	onClosed: destroy()
}
