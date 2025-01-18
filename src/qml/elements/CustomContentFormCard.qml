// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/**
 * This is a form card for custom content.
 */
FormCard.FormCard {
	default property alias customContent: contentArea.contentItem
	property alias title: header.title

	FormCard.FormHeader {
		id: header
	}

	Kirigami.Separator {
		Layout.fillWidth: true
	}

	FormCardCustomContentArea {
		id: contentArea
	}
}
