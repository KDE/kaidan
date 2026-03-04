// SPDX-FileCopyrightText: 2018 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

Kirigami.Dialog {
	id: root

	readonly property alias searchedText: root._searchedText
	property string _searchedText
	property alias gridView: gridView

	signal emojiSelected(string emoji)

	// Set a negative inset to fix the rounded corner of the dialog above the scroll bar.
	topInset: - Kirigami.Units.cornerRadius
	preferredWidth: largeButtonWidth
	preferredHeight: Kirigami.Units.gridUnit * 21
	maximumWidth: preferredWidth
	maximumHeight: Math.min(preferredHeight, applicationWindow().height - Kirigami.Units.gridUnit * 6)
	modal: false
	header: null
	footer: Controls.Control {
		background: Kirigami.ShadowedRectangle {
			color: primaryBackgroundColor
			border {
				color: tertiaryBackgroundColor
				width: 1
			}
			corners {
				bottomLeftRadius: Kirigami.Units.cornerRadius
				bottomRightRadius: Kirigami.Units.cornerRadius
			}
		}
		contentItem: RowLayout {
			id: footerLayout
			spacing: Kirigami.Units.smallSpacing
			visible: gridView.model.group !== Emoji.Group.Invalid

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
					hoverEnabled: true
					checkable: true
					checked: gridView.model.group === model.group
					background: InteractiveBackground {
						radius: Kirigami.Units.cornerRadius
					}
					contentItem: Text {
						text: model.label
						font.family: "emoji"
						font.pointSize: Kirigami.Units.iconSizes.small
						horizontalAlignment: Text.AlignHCenter
						verticalAlignment: Text.AlignVCenter
					}
					topInset: 0
					bottomInset: 0
					leftInset: 0
					rightInset: 0
					Layout.preferredWidth: implicitContentWidth * 2
					Layout.preferredHeight: implicitContentHeight * 2
					Layout.fillWidth: true
					onClicked: {
						gridView.currentIndex = 0
						gridView.model.group = model.group
					}
				}
			}
		}
		// The insets are set to "root.horizontalPadding" as a workaround in order to avoid that the footer overlaps the dialog.
		leftInset: root.horizontalPadding
		rightInset: root.horizontalPadding
		topPadding: Kirigami.Units.smallSpacing
		bottomPadding: Kirigami.Units.smallSpacing
		leftPadding: Kirigami.Units.smallSpacing
		rightPadding: Kirigami.Units.smallSpacing
	}
	onClosed: destroy()
	Component.onCompleted: {
		// Workaround since the first item would otherwise be as wide as gridView (for an unknown reason).
		gridView.model.group = Emoji.Group.Nature

		if (root.searchedText) {
			gridView.model.group = Emoji.Group.Invalid
		} else if (gridView.model.hasFavoriteEmojis) {
			gridView.model.group = Emoji.Group.Favorites
		} else {
			gridView.model.group = Emoji.Group.People
		}

		gridView.currentIndex = 0
	}

	GridView {
		id: gridView
		cellWidth: Kirigami.Units.iconSizes.smallMedium * 2.15
		cellHeight: Kirigami.Units.iconSizes.smallMedium * 2.15
		leftMargin: Kirigami.Units.smallSpacing
		rightMargin: Kirigami.Units.smallSpacing
		bottomMargin: - Kirigami.Units.smallSpacing
		implicitWidth: largeButtonWidth * 0.7
		model: EmojiProxyModel {
			sourceModel: EmojiModel {}
		}
		delegate: Controls.ItemDelegate {
			id: emojiDelegate

			property string emoji: model.unicode

			hoverEnabled: true
			checked: GridView.isCurrentItem
			background: InteractiveBackground {
				radius: Kirigami.Units.cornerRadius
			}
			contentItem: Text {
				text: emojiDelegate.emoji
				font.family: "emoji"
				font.pointSize: Kirigami.Units.iconSizes.smallMedium
			}
			topInset: 0
			bottomInset: 0
			leftInset: 0
			rightInset: 0
			Controls.ToolTip.text: model.shortName
			Controls.ToolTip.visible: hovered
			Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
			onClicked: selectEmoji(model.index, emojiDelegate.emoji)
		}
	}

	ColumnLayout {
		visible: !gridView.count

		Status {
			icon {
				source: "system-search-symbolic"
				Layout.preferredWidth: Kirigami.Units.iconSizes.huge
				Layout.preferredHeight: Kirigami.Units.iconSizes.huge
				Layout.maximumWidth: Kirigami.Units.iconSizes.huge
			}
			text {
				text: qsTr("No emoji found")
				type: Kirigami.Heading.Type.Secondary
			}
			opacity: 0.5
			Layout.topMargin: {
				root.height / 2 - height - implicitHeaderHeight / 2
			}
		}
	}

	function search(text) {
		if (text) {
			gridView.model.group = Emoji.Group.Invalid
		} else {
			gridView.model.group = gridView.model.hasFavoriteEmojis ? Emoji.Group.Favorites : Emoji.Group.People
		}

		_searchedText = text
		gridView.model.setFilterFixedString(searchedText.toLowerCase())
		gridView.currentIndex = 0
	}

	function selectCurrentItem() {
		selectEmoji(gridView.currentIndex, gridView.currentItem.emoji)
	}

	function selectEmoji(index, emoji) {
		gridView.model.addFavoriteEmoji(index)
		emojiSelected(emoji)
		close()
	}
}
