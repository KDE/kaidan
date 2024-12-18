// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import org.kde.kirigami 2.19 as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormSectionText {
	text: section
	padding: Kirigami.Units.largeSpacing
	leftPadding: Kirigami.Units.largeSpacing * 3
	font.weight: Font.Light
	anchors.left: parent.left
	anchors.right: parent.right
	background: Rectangle {
		color: tertiaryBackgroundColor
	}
}
