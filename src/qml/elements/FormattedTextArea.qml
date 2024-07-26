// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

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
