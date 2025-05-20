// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

CenteredAdaptiveText {
	property Account account

	text: qsTr("Some functionality will become visible once your account is enabled")
	font.weight: Font.Light
	background: Rectangle {
		color: secondaryBackgroundColor
	}
	visible: !account.settings.enabled
	topPadding: Kirigami.Units.largeSpacing * 2
	bottomPadding: Kirigami.Units.largeSpacing * 2
	leftPadding: Kirigami.Units.largeSpacing * 2
	rightPadding: Kirigami.Units.largeSpacing * 2
}
