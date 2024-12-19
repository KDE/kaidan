// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami

/**
 * This is a search field for filtering a list view.
 */
Kirigami.SearchField {
	id: root

	property ListView listView

	Keys.onEscapePressed: clear()
	Keys.onPressed: {
		switch (event.key) {
		case Qt.Key_Escape:
			clear()
			break
		case Qt.Key_Return:
		case Qt.Key_Enter:
			if (listView.count > 0) {
				// Simulate clicking on the first item of the listView.
				// E.g., it opens the chat of the first item in the roster list.
				// TODO: Remove case for DelegateRecycler as soon as the DelegateRecycler in the RosterPage is removed
				const firstItem = listView.itemAtIndex(0)
				if (firstItem instanceof Kirigami.DelegateRecycler) {
					firstItem.children[0].clicked()
				} else {
					firstItem.clicked()
				}
			}
		}
	}
}
