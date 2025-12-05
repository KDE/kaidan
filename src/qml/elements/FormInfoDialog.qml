// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Dialog {
	default property alias __data: mainArea.data

	// Set a negative inset to fix the rounded corners at the bottom of the dialog.
	bottomInset: - Kirigami.Units.cornerRadius
	preferredWidth: Kirigami.Units.gridUnit * 32

	ColumnLayout {
		id: mainArea
	}
}
