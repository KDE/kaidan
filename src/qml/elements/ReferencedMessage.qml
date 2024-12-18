// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

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
	readonly property real maximumDetailsWidth: maximumWidth - mainArea.leftPadding - avatar.width - contentArea.spacing - mainArea.rightPadding
	readonly property var geoCoordinate: Utils.geoCoordinate(root.body)

	Controls.Control {
		id: mainArea
		topPadding: Kirigami.Units.largeSpacing
		bottomPadding: topPadding
		leftPadding: root.quoteBarVisible ? Kirigami.Units.smallSpacing * 3 : Kirigami.Units.largeSpacing
		rightPadding: Kirigami.Units.largeSpacing
		background: Rectangle {
			color: root.backgroundColor
			radius: backgroundRadius

			Behavior on opacity {
				NumberAnimation {}
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
			spacing: Kirigami.Units.largeSpacing

			Avatar {
				id: avatar
				jid: root.senderId ? root.senderId : ChatController.accountJid
				name: root.senderName ? root.senderName : AccountManager.displayName
			}

			ColumnLayout {
				Controls.Label {
					text: senderNameTextMetrics.elidedText
					textFormat: Text.PlainText
					font.weight: Font.Medium
					font.italic: !root.senderName
					color: root.senderId ? Utils.userColor(root.senderId, root.senderName) : Utils.userColor(ChatController.accountJid, AccountManager.displayName)

					TextMetrics {
						id: senderNameTextMetrics
						text: root.senderName ? root.senderName : qsTr("Me")
						elide: Text.ElideRight
						elideWidth: root.maximumDetailsWidth
					}
				}

				FormattedTextEdit {
					text: bodyTextMetrics.elidedText
					color: Kirigami.Theme.disabledTextColor
					font.italic: !root.body || root.geoCoordinate.isValid
					Layout.minimumWidth: root.minimumWidth - mainArea.leftPadding - avatar.width - contentArea.spacing - mainArea.rightPadding

					TextMetrics {
						id: bodyTextMetrics
						text: {
							if (root.geoCoordinate.isValid) {
								return qsTr("Location")
							}

							return root.body ? Utils.removeNewLinesFromString(root.body) : qsTr("Media")
						}
						elide: Text.ElideRight
						elideWidth: root.maximumDetailsWidth
					}
				}
			}
		}

		OpacityChangingMouseArea {
			opacityItem: parent.background
			onClicked: root.jumpToMessage(MessageModel.searchMessageById(root.messageId))
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
