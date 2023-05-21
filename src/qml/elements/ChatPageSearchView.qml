// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2022 Tibor Csötönyi <dev@taibsu.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2023 Bhavy Airi <airiraghav@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

/**
 * This is a view for searching chat messages.
 */
Item {
	height: active ? searchField.height + 2 * Kirigami.Units.largeSpacing : 0
	clip: true
	visible: height != 0
	property bool active: false
	property alias searchFieldBusyIndicator: searchField.searchFieldBusyIndicator

	Behavior on height {
		SmoothedAnimation {
			velocity: 200
		}
	}

	// Background of the message search bar
	Rectangle {
		anchors.fill: parent
		color: Kirigami.Theme.backgroundColor
	}

	// Search field and its corresponding buttons
	RowLayout {
		// Anchoring like this binds it to the top of the chat page.
		// It makes it look like the search bar slides down from behind of the upper element.
		anchors.left: parent.left
		anchors.right: parent.right
		anchors.bottom: parent.bottom
		anchors.margins: Kirigami.Units.largeSpacing

		Controls.Button {
			text: qsTr("Close message search bar")
			icon.name: "window-close-symbolic"
			onClicked: searchBar.close()
			display: Controls.Button.IconOnly
			flat: true
		}

		Kirigami.SearchField {
			id: searchField
			Layout.fillWidth: true
			focusSequence: ""
			onVisibleChanged: text = ""
			onTextChanged: searchUpwardsFromBottom()
			onAccepted: searchFromCurrentIndex(true)
			Keys.onUpPressed: searchFromCurrentIndex(true)
			Keys.onDownPressed: searchFromCurrentIndex(false)
			Keys.onEscapePressed: close()
			autoAccept: false

			property alias searchFieldBusyIndicator: searchFieldBusyIndicator

			Controls.BusyIndicator {
				id: searchFieldBusyIndicator

				anchors {
					right: parent.right
					rightMargin: height / 2
					top: parent.top
					bottom: parent.bottom
				}

				running: false
			}
		}

		Controls.Button {
			text: qsTr("Search up")
			icon.name: "go-up-symbolic"
			display: Controls.Button.IconOnly
			flat: true
			onClicked: {
				searchFromCurrentIndex(true)
				searchField.forceActiveFocus()
			}
		}

		Controls.Button {
			text: qsTr("Search down")
			icon.name: "go-down-symbolic"
			display: Controls.Button.IconOnly
			flat: true
			onClicked: {
				searchFromCurrentIndex(false)
				searchField.forceActiveFocus()
			}
		}
	}

	/**
	 * Shows the search bar and focuses the search field.
	 */
	function open() {
		searchField.forceActiveFocus()
		active = true
	}

	/**
	 * Hides the search bar and resets the last search result.
	 */
	function close() {
		messageListView.currentIndex = -1
		active = false
	}

	/**
	 * Searches upwards for a message containing the entered text in the search field starting from the current index of the message list view.
	 */
	function searchUpwardsFromBottom() {
		search(true, 0)
	}

	/**
	 * Searches for a message containing the entered text in the search field starting from the current index of the message list view.
	 *
	 * The searchField is automatically focused again on desktop devices if it lost focus (e.g., after clicking a button).
	 *
	 * @param searchUpwards true for searching upwards or false for searching downwards
	 */
	function searchFromCurrentIndex(searchUpwards) {
		if (!Kirigami.Settings.isMobile && !searchField.activeFocus)
			searchField.forceActiveFocus()

		search(searchUpwards, messageListView.currentIndex + (searchUpwards ? 1 : -1))
	}

	/**
	 * Searches for a message containing the entered text in the search field.
	 *
	 * If a message is found for the entered text, that message is highlighted.
	 *
	 * @param searchUpwards true for searching upwards or false for searching downwards
	 * @param startIndex index of the first message to search for the entered text
	 */
	function search(searchUpwards, startIndex) {
		let newIndex = -1
		const searchedString = searchField.text

		if (searchedString.length > 0) {
			searchFieldBusyIndicator.running = true

			if (searchUpwards) {
				if (startIndex === 0) {
					messageListView.currentIndex = MessageModel.searchForMessageFromNewToOld(searchedString)
				} else {
					newIndex = MessageModel.searchForMessageFromNewToOld(searchedString, startIndex)
					if (newIndex !== -1) {
						messageListView.currentIndex = newIndex
					}
				}

				if (messageListView.currentIndex !== -1) {
					searchFieldBusyIndicator.running = false
				}
			} else {
				newIndex = MessageModel.searchForMessageFromOldToNew(searchedString, startIndex)

				if (newIndex !== -1) {
					messageListView.currentIndex = newIndex
				}

				searchFieldBusyIndicator.running = false
			}
		}
	}
}
