// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2017 Ilya Bizyaev <bizyaev@zoho.com>
// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Xavier <xavi@delape.net>
// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2019 caca hueto <cacahueto@olomono.de>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Yury Gubich <blue@macaw.me>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
// SPDX-FileCopyrightText: 2023 Bhavy Airi <airiraghav@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

Controls.ItemDelegate {
	id: root

	property ToolbarButton messageSearchButton
	property ListView messageListView
	property ChatPageSendingPane sendingPane
	property MessageReactionEmojiPicker reactionEmojiPicker
	property ChatController chatController
	property int modelIndex
	property string msgId
	property string senderJid
	property string groupChatSenderId
	property string senderName
	property string chatJid
	property string chatName
	property bool isOwn
	property bool isGroupChatMessage
	property string replyToJid
	property string replyToGroupChatParticipantId
	property string replyToName
	property string replyId
	property string replyQuote
	property int encryption
	property int trustLevel
	property string messageBody
	property string date
	property string time
	property int deliveryState
	property string deliveryStateName
	property string deliveryStateIcon
	property bool isLastReadOwnMessage
	property bool isLatestOldMessage
	property bool edited
	property bool isSpoiler
	property string spoilerHint
	property bool isShowingSpoiler: false
	property string errorText: ""
	property var files
	property var displayedReactions
	property var detailedReactions
	property bool ownReactionsFailed
	property string groupChatInvitationJid
	property var geoCoordinate
	property bool marked
	property bool isGroupBegin: determineMessageGroupDelimiter(messageListView.count - 1, 1)
	property bool isGroupEnd: determineMessageGroupDelimiter()
	property real bubblePadding: Kirigami.Units.smallSpacing
	property real maximumBubbleContentWidth: width - Kirigami.Units.largeSpacing * (root.isGroupChatMessage && !root.isOwn ? 14 : 8 + (markedMessageArea.visible ? 2 : 0))
	property ChatMessageContextMenu contextMenu

	width: messageListView.width
	height: messageArea.implicitHeight + (isGroupEnd ? Kirigami.Units.largeSpacing : Kirigami.Units.smallSpacing)
	background: null
	contentItem: ColumnLayout {
		id: messageArea
		spacing: -5

		RowLayout {
			// Own messages are on the right, others on the left side.
			layoutDirection: root.isOwn ? Qt.RightToLeft : Qt.LeftToRight
			spacing: 0

			Avatar {
				id: avatar
				jid: root.senderJid ? root.senderJid : root.groupChatSenderId
				name: root.senderName
				visible: root.isGroupChatMessage && !root.isOwn && root.isGroupBegin
				Layout.alignment: Qt.AlignTop

				Kirigami.ShadowedRectangle {
					z: parent.z - 0.1
					shadow {
						color: Kirigami.Theme.disabledTextColor
						size: Kirigami.Units.largeSpacing
					}
					radius: height / 2
					opacity: avatarMouseArea.containsMouse ? 1 : 0
					anchors.fill: parent

					Behavior on opacity {
						NumberAnimation {}
					}
				}

				MouseArea {
					id: avatarMouseArea
					hoverEnabled: true
					anchors.fill: parent
					onClicked: sendingPane.messageArea.mentionParticipant(Utils.groupChatUserMentionPrefix + root.senderName)
				}
			}

			// message bubble
			Controls.Control {
				id: bubble

				readonly property string paddingText: messageReactionArea.visible ? "" : Utils.messageBubblePaddingCharacter.repeat(Math.ceil(background.metaInfo.width / background.dummy.implicitWidth))
				readonly property alias backgroundColor: bubbleBackground.color

				topPadding: 0
				bottomPadding: root.bubblePadding
				leftPadding: root.isOwn ? root.bubblePadding : root.bubblePadding + background.tailSize
				rightPadding: root.isOwn ? root.bubblePadding + background.tailSize : root.bubblePadding
				Layout.leftMargin: root.isGroupChatMessage && !root.isOwn && !avatar.visible ? avatar.width : 0
				background: MessageBackground {
					id: bubbleBackground
					message: root
					showTail: root.isGroupBegin
					highlighted: root.messageListView.currentIndex === root.modelIndex

					MouseArea {
						anchors.fill: parent
						acceptedButtons: Qt.LeftButton | Qt.RightButton
						onClicked: mouse => {
							if (mouse.button === Qt.RightButton) {
								root.showContextMenu(this)
							}
						}
						onPressAndHold: root.showContextMenu(this)
					}
				}
				contentItem: ColumnLayout {
					Controls.Label {
						text: root.senderName
						color: Utils.userColor(root.senderJid ? root.senderJid : root.groupChatSenderId, root.senderName)
						font.weight: Font.Medium
						visible: root.isGroupChatMessage && !root.isOwn && root.isGroupBegin && text.length
						topPadding: root.bubblePadding
						bottomPadding: root.messageBody && !root.replyId && !root.files.length ? - topPadding : topPadding
						leftPadding: topPadding
						rightPadding: topPadding
					}

					Loader {
						id: referencedMessageLoader
						sourceComponent: root.replyId ? referencedMessage : undefined
						Layout.topMargin: item ? root.bubblePadding : 0

						Component {
							id: referencedMessage

							ReferencedMessage {
								account: root.chatController.account
								senderId: root.replyToJid ? root.replyToJid : root.replyToGroupChatParticipantId
								senderName: root.replyToName
								messageId: root.replyId
								body: root.replyQuote
								messageListView: root.messageListView
								minimumWidth: Math.max(spoilerHintArea.width, bodyArea.width + bodyArea.Layout.margins * 2, messageReactionArea.width, bubbleBackground.metaInfo.width)
								maximumWidth: root.maximumBubbleContentWidth
								backgroundColor: root.isOwn ? primaryBackgroundColor : secondaryBackgroundColor
							}
						}
					}

					Repeater {
						id: mediaList
						model: root.files
						delegate: Loader {
							property var file: modelData
							property real minimumWidth: Math.max(parent.width, referencedMessageLoader.item ? referencedMessageLoader.item.width : 0, spoilerHintArea.width, bodyArea.width + bodyArea.Layout.margins * 2, messageReactionArea.width, bubbleBackground.metaInfo.width)
							property real maximumWidth: root.maximumBubbleContentWidth
							property color mainAreaBackgroundColor: root.isOwn ? primaryBackgroundColor : secondaryBackgroundColor

							sourceComponent: file.locallyAvailable && file.transferState === File.TransferState.Done && file.type === Enums.MessageType.MessageAudio && !file.description ? audio : mediumMessagePreview

							Component {
								id: mediumMessagePreview

								MediumMessagePreview {
									file: parent.file
									message: root
									minimumWidth: parent.minimumWidth
									maximumWidth: parent.maximumWidth
									mainAreaBackground.color: parent.mainAreaBackgroundColor
								}
							}

							Component {
								id: audio

								Audio {
									file: parent.file
									message: root
									minimumWidth: parent.minimumWidth
									maximumWidth: parent.maximumWidth
									mainAreaBackground.color: parent.mainAreaBackgroundColor
								}
							}
						}
					}

					ColumnLayout {
						id: spoilerHintArea
						visible: root.isSpoiler
						Layout.minimumWidth: bubbleBackground.metaInfo.width
						Layout.leftMargin: root.bubblePadding
						Layout.rightMargin: Layout.leftMargin

						RowLayout {
							Layout.topMargin: root.bubblePadding
							Layout.bottomMargin: Layout.topMargin

							FormattedTextEdit {
								text: root.spoilerHint ? root.spoilerHint : qsTr("Spoiler")
								enabled: true
								enhancedFormatting: true
								Layout.fillWidth: true
							}

							ClickableIcon {
								source: root.isShowingSpoiler ? "password-show-off" : "password-show-on"
								Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
								Layout.leftMargin: Kirigami.Units.largeSpacing
								onClicked: root.isShowingSpoiler = !root.isShowingSpoiler
							}
						}

						Kirigami.Separator {
							visible: root.isShowingSpoiler
							Layout.fillWidth: true
							color: {
								const bgColor = Kirigami.Theme.backgroundColor
								const textColor = Kirigami.Theme.textColor
								return Qt.tint(textColor, Qt.rgba(bgColor.r, bgColor.g, bgColor.b, 0.7))
							}
						}
					}

					Item {
						visible: (!root.messageBody || (root.isSpoiler && !root.isShowingSpoiler)) && !messageReactionArea.visible
						height: bubbleBackground.metaInfo.height
					}

					ColumnLayout {
						id: bodyArea
						visible: bodyAreaLoader.item

						Loader {
							id: bodyAreaLoader
							sourceComponent: {
								if (root.groupChatInvitationJid) {
									return groupChatInvitationComponent
								}

								if (root.geoCoordinate.isValid) {
									if (root.chatController.account.settings.geoLocationMapPreviewEnabled) {
										return geoLocationMapPreview
									}

									return geoLocationPreview
								}

								if (root.messageBody && (!root.isSpoiler || root.isShowingSpoiler)) {
									return bodyComponent
								}

								return undefined
							}
							Layout.bottomMargin: (root.groupChatInvitationJid || root.geoCoordinate.isValid) && !messageReactionArea.visible ? bubbleBackground.metaInfo.height + bodyArea.parent.spacing : 0

							Component {
								id: bodyComponent

								RowLayout {
									FormattedTextEdit {
										text: root.messageBody + bubble.paddingText
										enabled: true
										visible: messageBody
										enhancedFormatting: true
										padding: root.bubblePadding
										Layout.maximumWidth: root.maximumBubbleContentWidth
									}
								}
							}

							Component {
								id: groupChatInvitationComponent

								GroupChatInvitation {
									message: root
									minimumWidth: Math.max(referencedMessageLoader.item ? referencedMessageLoader.item.width : 0, bodyArea.parent.width, spoilerHintArea.width, messageReactionArea.width, bubbleBackground.metaInfo.width)
									maximumWidth: root.maximumBubbleContentWidth
									mainAreaBackground.color: root.isOwn ? primaryBackgroundColor : secondaryBackgroundColor
								}
							}

							Component {
								id: geoLocationPreview

								GeoLocationPreview {
									message: root
									minimumWidth: Math.max(referencedMessageLoader.item ? referencedMessageLoader.item.width : 0, bodyArea.parent.width, spoilerHintArea.width, messageReactionArea.width, bubbleBackground.metaInfo.width)
									maximumWidth: root.maximumBubbleContentWidth
									mainAreaBackground.color: root.isOwn ? primaryBackgroundColor : secondaryBackgroundColor
								}
							}

							Component {
								id: geoLocationMapPreview

								RowLayout {
									GeoLocationMapPreview {
										center: root.geoCoordinate
										message: root
										Layout.preferredWidth: Kirigami.Units.gridUnit * 30
										Layout.preferredHeight: Kirigami.Units.gridUnit * 20
										Layout.maximumWidth: root.maximumBubbleContentWidth
									}
								}
							}
						}
					}

					// message reactions (emojis in reaction to this message)
					Flow {
						id: messageReactionArea
						visible: displayedReactionsArea.count
						spacing: Kirigami.Units.smallSpacing
						Layout.bottomMargin: Kirigami.Units.smallSpacing * 5
						Layout.maximumWidth: root.maximumBubbleContentWidth
						Layout.preferredWidth: {
							const detailsButtonLoaded = messageReactionDetailsButtonLoader.status === Loader.Ready
							var buttonCount = 1

							if (messageReactionDetailsButtonLoader.status === Loader.Ready) {
								buttonCount++
							}

							if (messageReactionRetryButtonLoader.status === Loader.Ready) {
								buttonCount++
							}

							const totalSpacing = spacing * (displayedReactionsArea.count + buttonCount - 1)
							var displayedReactionsWidth = 0

							for (var i = 0; i < displayedReactionsArea.count; i++) {
								displayedReactionsWidth += displayedReactionsArea.itemAt(i).width
							}

							return displayedReactionsWidth + (messageReactionAdditionButton.width * buttonCount) + totalSpacing
						}

						Repeater {
							id: displayedReactionsArea
							model: root.displayedReactions

							MessageReactionDisplayButton {
								text: modelData.emoji
								accentColor: bubble.backgroundColor
								messageId: root.msgId
								count: modelData.count
								deliveryState: modelData.deliveryState
								ownReactionIncluded: modelData.ownReactionIncluded
								isOwnMessage: root.isOwn
								messageModel: root.chatController.messageModel
							}
						}

						MessageReactionAdditionButton {
							id: messageReactionAdditionButton
							accentColor: bubble.backgroundColor
							messageId: root.msgId
							isOwnMessage: root.isOwn
							emojiPicker: root.reactionEmojiPicker
						}

						Loader {
							id: messageReactionDetailsButtonLoader
							sourceComponent: root.detailedReactions && root.detailedReactions.length ? messageReactionDetailsButton : undefined

							Component {
								id: messageReactionDetailsButton

								MessageReactionDetailsButton {
									account: root.chatController.account
									chatJid: root.chatController.jid
									accentColor: bubble.backgroundColor
									isOwnMessage: root.isOwn
									reactions: root.detailedReactions
								}
							}
						}

						Loader {
							id: messageReactionRetryButtonLoader
							sourceComponent: root.ownReactionsFailed ? messageReactionRetryButton : undefined

							Component {
								id: messageReactionRetryButton

								MessageReactionRetryButton {
									accentColor: bubble.backgroundColor
									messageId: root.msgId
									isOwnMessage: root.isOwn
									messageModel: root.chatController.messageModel
								}
							}
						}
					}
				}
			}

			RowLayout {
				id: markedMessageArea
				spacing: Kirigami.Units.smallSpacing
				opacity: {
					if (!root.marked) {
						return 0
					}

					if (messageUnmarkingMouseArea.pressed) {
						return 0.3
					}

					if (messageUnmarkingMouseArea.containsMouse) {
						return 0.6
					}

					return 0.9
				}
				visible: opacity
				layoutDirection: parent.layoutDirection
				Layout.leftMargin: root.isOwn ? 0 : Kirigami.Units.smallSpacing
				Layout.rightMargin: root.isOwn ? Kirigami.Units.smallSpacing : 0

				Behavior on opacity {
					NumberAnimation {}
				}

				Rectangle {
					id: markedMessageBar
					color: Kirigami.Theme.neutralTextColor
					radius: height / 2
					width: Kirigami.Units.smallSpacing
					Layout.fillHeight: true
				}

				Rectangle {
					color: "transparent"
					border.color: markedMessageBar.color
					radius: height / 2
					width: Kirigami.Units.gridUnit * 2
					height: width

					Rectangle {
						color: markedMessageBar.color
						radius: height / 2
						width: Kirigami.Units.gridUnit
						height: width
						anchors.centerIn: parent
					}

					MouseArea {
						id: messageUnmarkingMouseArea
						anchors.fill: parent
						hoverEnabled: true
						onClicked: mouse => {
							if (mouse.button === Qt.LeftButton) {
								root.chatController.messageModel.setMessageMarked(root.modelIndex, false)
							}
						}
					}
				}
			}

			// placeholder
			Item {
				Layout.fillWidth: true
			}
		}

		// Read marker text for contact message
		RowLayout {
			visible: root.isLatestOldMessage
			spacing: Kirigami.Units.smallSpacing * 3
			Layout.topMargin: spacing
			Layout.leftMargin: spacing
			Layout.rightMargin: spacing

			Kirigami.Separator {
				color: Kirigami.Theme.highlightColor
				opacity: 0.8
				Layout.fillWidth: true
			}

			ScalableText {
				text: qsTr("New")
				color: Kirigami.Theme.highlightColor
				scaleFactor: 0.9
				elide: Text.ElideMiddle
				Layout.maximumWidth: parent.width - Kirigami.Units.largeSpacing * 10
			}

			Kirigami.Separator {
				color: Kirigami.Theme.highlightColor
				opacity: 0.8
				Layout.fillWidth: true
			}
		}

		// Read marker text for own message
		RowLayout {
			visible: root.isLastReadOwnMessage && root.chatJid !== root.chatController.account.settings.jid
			spacing: Kirigami.Units.smallSpacing * 3
			Layout.topMargin: spacing
			Layout.leftMargin: spacing
			Layout.rightMargin: spacing

			Kirigami.Separator {
				opacity: 0.8
				Layout.fillWidth: true
			}

			ScalableText {
				text: qsTr("%1 has read up to this point").arg(chatName)
				color: Kirigami.Theme.disabledTextColor
				scaleFactor: 0.9
				elide: Text.ElideMiddle
				Layout.maximumWidth: parent.width - Kirigami.Units.largeSpacing * 10
			}

			Kirigami.Separator {
				opacity: 0.8
				Layout.fillWidth: true
			}
		}

		Component {
			id: contextMenu

			ChatMessageContextMenu {
				message: root
				Component.onCompleted: root.contextMenu = this
				Component.onDestruction: root.contextMenu = null
			}
		}

		Component {
			id: geoLocationMapPage

			GeoLocationMapPage {
				coordinate: root.geoCoordinate
			}
		}
	}
	Kirigami.Theme.inherit: false

	function determineMessageGroupDelimiter(delimitingIndex = 0, indexOffset = -1) {
		if (modelIndex < 0 || delimitingIndex < 0 || modelIndex === delimitingIndex) {
			return true
		}

		const nextMessageIndex = chatController.messageModel.index(modelIndex + indexOffset, 0)

		if (isOwn) {
			return !chatController.messageModel.data(nextMessageIndex, MessageModel.IsOwn)
		}

		if (senderJid) {
			return chatController.messageModel.data(nextMessageIndex, MessageModel.SenderJid) !== senderJid
		}

		return chatController.messageModel.data(nextMessageIndex, MessageModel.GroupChatSenderId) !== groupChatSenderId
	}

	function showContextMenu(mouseArea, file) {
		messageSearchButton.checked = false
		root.messageListView.currentIndex = root.modelIndex
		contextMenu.createObject().show(mouseArea, file)
	}

	function openGeoLocationMap() {
		if (root.chatController.account.openGeoLocation(geoCoordinate)) {
			openPage(geoLocationMapPage)
		}
	}
}
