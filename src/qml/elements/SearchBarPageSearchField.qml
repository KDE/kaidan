// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

/**
 * This is the search field of the SearchBarPage for filtering a list.
 */
Kirigami.SearchField {
	id: root

	property ListView listView

	onTextChanged: listView.model.setFilterFixedString(text.toLowerCase())
	Keys.onEscapePressed: text = ""

	onActiveFocusChanged: {
		if (activeFocus) {
			selectAll()
		}
	}

	Keys.onPressed: {
		switch (event.key) {
		case Qt.Key_Escape:
			text = ""
			break
		case Qt.Key_Return:
		case Qt.Key_Enter:
			if (listView.count > 0) {
				// Simulate clicking on the first item of the listView.
				// E.g., it opens the chat of the first item in the roster list.
				// TODO: Remove ".children[0]" as soon as the DelegateRecycler in the RosterPage is removed
				listView.itemAtIndex(0).children[0].clicked()
			}
		}
	}
}
