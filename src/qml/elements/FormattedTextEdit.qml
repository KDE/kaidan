// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

/**
 * Correctly formatted text including proper emojis and links.
 *
 * This is intended to display text.
 * For entering text, use FormattedTextArea.
 */
TextEdit {
	// Whether to apply an enhanced formatting (e.g., single emojis are enlarged and links are
	// marked as such)
	property bool enhancedFormatting: false

	color: Kirigami.Theme.textColor
	enabled: false
	wrapMode: Text.Wrap
	readOnly: true
	activeFocusOnPress: false
	onLinkActivated: Qt.openUrlExternally(link)
	Component.onCompleted: {
		if (enhancedFormatting) {
			Utils.attachEnhancedTextFormatting(textDocument)
		} else {
			Utils.attachTextFormatting(textDocument)
		}
	}

	HoverHandler {
		id: hoverHandler
		cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
	}
}
