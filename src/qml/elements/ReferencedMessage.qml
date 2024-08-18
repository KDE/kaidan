// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

RowLayout {
	id: root

	property string senderId
	property string senderName
	property string messageId
	property string body
	property ListView messageListView
	property real minimumWidth
	property real maximumWidth
	property color backgroundColor: secondaryBackgroundColor
	property real backgroundRadius: roundedCornersRadius
	property bool quoteBarVisible: true

	Controls.Control {
		topPadding: Kirigami.Units.largeSpacing
		bottomPadding: topPadding
		leftPadding: root.quoteBarVisible ? Kirigami.Units.largeSpacing : Kirigami.Units.smallSpacing
		rightPadding: Kirigami.Units.largeSpacing
		background: Rectangle {
			color: root.backgroundColor
			radius: backgroundRadius

			Behavior on opacity {
				OpacityAnimator {
					duration: Kirigami.Units.shortDuration
				}
			}

			Kirigami.ShadowedRectangle {
				color: Kirigami.Theme.highlightColor
				visible: root.quoteBarVisible
				width: 4
				opacity: 0.6
				corners {
					topLeftRadius: parent.radius
					topRightRadius: 0
					bottomRightRadius: 0
					bottomLeftRadius: parent.radius
				}
				anchors {
					top: parent.top
					bottom: parent.bottom
					left: parent.left
				}
			}
		}
		contentItem: RowLayout {
			id: contentArea

			Avatar {
				id: avatar
				jid: root.senderId ? root.senderId : ChatController.accountJid
				name: root.senderName ? root.senderName : AccountManager.displayName
				Layout.preferredHeight: Kirigami.Units.gridUnit * 2
			}

			ColumnLayout {
				Controls.Label {
					text: root.senderName ? root.senderName : qsTr("Me")
					textFormat: Text.PlainText
					font.weight: Font.Medium
					font.italic: !root.senderName
					elide: Text.ElideRight
					maximumLineCount: 1
					color: root.senderId ? Utils.userColor(root.senderId, root.senderName) : Utils.userColor(ChatController.accountJid, AccountManager.displayName)
				}

				FormattedTextEdit {
					text: root.body ? bodyTextMetrics.elidedText : qsTr("Media")
					color: Kirigami.Theme.disabledTextColor
					font.italic: !root.body
					Layout.minimumWidth: root.minimumWidth - contentArea.parent.leftPadding - avatar.width - contentArea.spacing * 2 - contentArea.parent.rightPadding

					TextMetrics {
						id: bodyTextMetrics
						text: Utils.removeNewLinesFromString(root.body)
						elide: Text.ElideRight
						elideWidth: root.maximumWidth - avatar.width - contentArea.spacing * 2
					}
				}
			}

			Item {
				Layout.fillWidth: true
			}
		}

		MouseArea {
			cursorShape: Qt.PointingHandCursor
			hoverEnabled: true
			anchors.fill: parent
			onContainsMouseChanged: parent.background.opacity = containsMouse ? 0.5 : 1
			onPressed: root.jumpToMessage(MessageModel.searchMessageById(root.messageId))
		}
	}

	Connections {
		target: MessageModel

		function onMessageSearchByIdInDbFinished(foundMessageIndex) {
			root.jumpToMessage(foundMessageIndex)
		}
	}

	function jumpToMessage(messageIndex) {
		messageListView.positionViewAtIndex(messageIndex, ListView.End)
		messageListView.highlightShortly(messageIndex)
	}
}
