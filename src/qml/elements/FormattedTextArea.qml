// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

/**
 * Area for displaying correctly formatted text including proper emojis.
 *
 * This is intended to enter text.
 * For displaying text only, use FormattedTextEdit.
 */
Controls.TextArea {
	id: root
	wrapMode: Text.Wrap

	TextFormatter {
		textDocument: root.textDocument
	}
}
