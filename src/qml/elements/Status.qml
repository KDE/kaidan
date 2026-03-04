// SPDX-FileCopyrightText: 2026 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/**
 * This is a hint for the status of an action.
 *
 * The icon and text are positioned in the center without additional spacing between them while
 * allowing the text's end to be elided.
 */
ColumnLayout {
	property alias icon: icon
	property alias text: text

	Kirigami.Icon {
		id: icon
		Layout.maximumWidth: Kirigami.Units.iconSizes.enormous
		Layout.maximumHeight: Kirigami.Units.iconSizes.enormous
		Layout.fillWidth: true
		Layout.fillHeight: true
		Layout.alignment: Qt.AlignHCenter
	}

	Kirigami.Heading {
		id: text
		wrapMode: Text.Wrap
		elide: Text.ElideRight
		horizontalAlignment: Text.AlignHCenter
		Layout.fillWidth: true
		// "Layout.fillHeight: true" cannot be used to position the icon and text in the center
		// without additional spacing between them while keeping the text's end being elided.
		Layout.fillHeight: icon.height + parent.spacing + implicitHeight > parent.parent.height
	}
}
