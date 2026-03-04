// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

/**
 * This is an emoji picker for adding emojis in reaction to a message.
 */
EmojiPicker {
	id: root

	property string messageId
	property MessageModel messageModel

	parent: root.parent
	anchors.centerIn: parent
	topInset: 0
	header: Controls.Control {
		background: Kirigami.ShadowedRectangle {
			color: primaryBackgroundColor
			border {
				color: tertiaryBackgroundColor
				width: 1
			}
			corners {
				topLeftRadius: Kirigami.Units.cornerRadius
				topRightRadius: Kirigami.Units.cornerRadius
			}
		}
		contentItem: ListViewSearchField {
			id: searchField
			onAccepted: root.search(text)
			Keys.onDownPressed: {
				if (root.gridView.currentIndex === root.gridView.count - 1) {
					root.gridView.currentIndex = 0
				} else {
					root.gridView.moveCurrentIndexDown()
				}
			}
			Keys.onUpPressed: {
				if (root.gridView.currentIndex === 0) {
					root.gridView.currentIndex = root.gridView.count - 1
				} else {
					root.gridView.moveCurrentIndexUp()
				}
			}
			Keys.onLeftPressed: {
					if (root.gridView.currentIndex === 0) {
						root.gridView.currentIndex = root.gridView.count - 1
					} else {
						root.gridView.moveCurrentIndexLeft()
					}
			}
			Keys.onRightPressed: {
					if (root.gridView.currentIndex === root.gridView.count - 1) {
						root.gridView.currentIndex = 0
					} else {
						root.gridView.moveCurrentIndexRight()
					}
			}
			Keys.onReturnPressed: root.selectCurrentItem()
		}
		// The insets are set to "root.horizontalPadding" as a workaround in order to avoid that the footer overlaps the dialog.
		leftInset: root.horizontalPadding
		rightInset: root.horizontalPadding
	}
	gridView.topMargin: Kirigami.Units.smallSpacing
	onEmojiSelected: emoji => messageModel.addMessageReaction(messageId, emoji)
	onOpened: searchField.forceActiveFocus()
}
