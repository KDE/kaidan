/*
 *   SPDX-FileCopyrightText: 2022 Nate Graham <nate@kde.org>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2

import org.kde.kirigami 2.19 as Kirigami

QQC2.Button {
	id: root

	required property string buttonIcon
	required property string title
	property string tooltipText

	implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
							implicitContentWidth + leftPadding + rightPadding)
	implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
							 implicitContentHeight + topPadding + bottomPadding)

	leftPadding: Kirigami.Units.largeSpacing
	rightPadding: Kirigami.Units.largeSpacing
	topPadding: Kirigami.Units.largeSpacing
	bottomPadding: Kirigami.Units.largeSpacing

	contentItem: ColumnLayout {
		spacing: 0

		// Icon
		Kirigami.Icon {
			Layout.preferredWidth: Kirigami.Units.iconSizes.medium
			Layout.preferredHeight: Kirigami.Units.iconSizes.medium
			Layout.bottomMargin: Kirigami.Units.largeSpacing
			Layout.alignment: Qt.AlignLeft
			source: root.buttonIcon
		}
		// Title
		Kirigami.Heading {
			Layout.fillWidth: true
			level: 4
			text: root.title
			elide: Text.ElideRight
			wrapMode: Text.Wrap
		}

	}
	QQC2.ToolTip {
		text: root.tooltipText ? root.tooltipText : ""
	}
}
