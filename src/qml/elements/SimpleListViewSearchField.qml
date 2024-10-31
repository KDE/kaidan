// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * This is a search field for filtering a list view via a simple search.
 */
ListViewSearchField {
	id: root
	onTextChanged: listView.model.setFilterFixedString(text.toLowerCase())
}
