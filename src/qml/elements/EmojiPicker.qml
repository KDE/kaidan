// SPDX-FileCopyrightText: 2018 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14

import org.kde.kirigami 2.19 as Kirigami
import EmojiModel 0.1

Controls.Popup {
	id: root
	width: Kirigami.Units.gridUnit * 20
	height: Kirigami.Units.gridUnit * 15

	property Controls.TextArea textArea
	property string searchedText

	ColumnLayout {
		anchors.fill: parent

		GridView {
			id: emojiView

			Layout.fillWidth: true
			Layout.fillHeight: true

			cellWidth: Kirigami.Units.gridUnit * 2.33
			cellHeight: cellWidth

			boundsBehavior: Flickable.DragOverBounds
			clip: true

			model: EmojiProxyModel {
				sourceModel: EmojiModel {}
				group: hasFavoriteEmojis ? Emoji.Group.Favorites : Emoji.Group.People
			}

			delegate: Controls.ItemDelegate {
				width: emojiView.cellWidth
				height: emojiView.cellHeight
				hoverEnabled: true
				Controls.ToolTip.text: model.shortName
				Controls.ToolTip.visible: hovered
				Controls.ToolTip.delay: Kirigami.Units.toolTipDelay

				contentItem: Text {
					horizontalAlignment: Text.AlignHCenter
					verticalAlignment: Text.AlignVCenter

					font.pointSize: 20
					text: model.unicode
				}

				onClicked: {
					emojiView.model.addFavoriteEmoji(model.index);
					textArea.remove(textArea.cursorPosition - searchedText.length, textArea.cursorPosition)
					textArea.insert(textArea.cursorPosition, model.unicode + " ")
					close()
				}
			}

			Controls.ScrollBar.vertical: Controls.ScrollBar {}
		}

		Rectangle {
			visible: emojiView.model.group !== Emoji.Group.Invalid
			color: Kirigami.Theme.highlightColor
			Layout.fillWidth: true
			Layout.preferredHeight: 2
		}

		Row {
			visible: emojiView.model.group !== Emoji.Group.Invalid

			Repeater {
				model: ListModel {
					ListElement { label: "🔖"; group: Emoji.Group.Favorites }
					ListElement { label: "🙂"; group: Emoji.Group.People }
					ListElement { label: "🌲"; group: Emoji.Group.Nature }
					ListElement { label: "🍛"; group: Emoji.Group.Food }
					ListElement { label: "🚁"; group: Emoji.Group.Activity }
					ListElement { label: "🚅"; group: Emoji.Group.Travel }
					ListElement { label: "💡"; group: Emoji.Group.Objects }
					ListElement { label: "🔣"; group: Emoji.Group.Symbols }
					ListElement { label: "🏁"; group: Emoji.Group.Flags }
				}

				delegate: Controls.ItemDelegate {
					width: Kirigami.Units.gridUnit * 2.08
					height: width
					hoverEnabled: true
					highlighted: emojiView.model.group === model.group

					contentItem: Text {
						horizontalAlignment: Text.AlignHCenter
						verticalAlignment: Text.AlignVCenter

						font.pointSize: 20
						text: model.label
					}

					onClicked: emojiView.model.group = model.group
				}
			}
		}
	}

	onClosed: clearSearch()

	function toggle() {
		if (!visible || isSearchActive())
			openWithFavorites()
		else
			close()
	}

	function openWithFavorites() {
		clearSearch()
		open()
	}

	function openForSearch(currentCharacter) {
		searchedText += currentCharacter
		emojiView.model.group = Emoji.Group.Invalid
		open()
	}

	function search() {
		emojiView.model.filter = searchedText.toLowerCase()
	}

	function isSearchActive() {
		return emojiView.model.group === Emoji.Group.Invalid
	}

	function clearSearch() {
		searchedText = ""
		search()
		setFavoritesAsDefaultIfAvailable()
	}

	function setFavoritesAsDefaultIfAvailable() {
		emojiView.model.group = emojiView.model.hasFavoriteEmojis ? Emoji.Group.Favorites : Emoji.Group.People
	}
}
