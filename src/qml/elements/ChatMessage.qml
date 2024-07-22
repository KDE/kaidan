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

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls
import QtGraphicalEffects 1.15
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0
import MediaUtils 0.1

Kirigami.SwipeListItem {
	id: root

	property Controls.Menu contextMenu
	property MessageReactionEmojiPicker reactionEmojiPicker
	property MessageReactionDetailsSheet reactionDetailsSheet

	property int modelIndex
	property string msgId
	property string senderId
	property string senderName
	property string chatName
	property bool isOwn: true
	property bool isGroupChatMessage
	property int encryption
	property int trustLevel
	property string messageBody
	property string date
	property string time
	property int deliveryState
	property string deliveryStateName
	property url deliveryStateIcon
	property bool isLastReadOwnMessage
	property bool isLastReadContactMessage
	property bool edited
	property bool isSpoiler
	property string spoilerHint
	property bool isShowingSpoiler: false
	property string errorText: ""
	property var files;
	property var displayedReactions
	property var detailedReactions
	property bool ownReactionsFailed
	property var groupChatInvitationJid

	property bool isGroupBegin: modelIndex === MessageModel.rowCount() - 1 || MessageModel.data(MessageModel.index(modelIndex + 1, 0), MessageModel.SenderId) !== senderId
	property bool isGroupEnd: modelIndex < 1 || MessageModel.data(MessageModel.index(modelIndex - 1, 0), MessageModel.SenderId) !== senderId

	signal messageEditRequested(string replaceId, string body, string spoilerHint)
	signal quoteRequested(string body)

	height: messageArea.implicitHeight + (isGroupEnd ? Kirigami.Units.largeSpacing : Kirigami.Units.smallSpacing)
	alwaysVisibleActions: false

	actions: [
		// TODO: Move message to the left when action is displayed and message is too large or
		// display all actions at the bottom / at the top of the message bubble
		Kirigami.Action {
			text: "Add message reaction"
			icon.name: "smiley-add"
			visible: {
				// Do not allow to send reactions if the message has not yet got a stanza ID from
				// the server to be referenced by.
				if (root.isGroupChatMessage &&
					root.deliveryState !== Enums.Sent &&
					root.deliveryState !== Enums.Delivered) {
					return false
				}

				return !root.displayedReactions.length && !root.groupChatInvitationJid
			}
			onTriggered: {
				root.reactionEmojiPicker.messageId = root.msgId
				root.reactionEmojiPicker.open()
			}
		}
	]

	ColumnLayout {
		id: messageArea
		spacing: -5

		RowLayout {
			// Own messages are on the right, others on the left side.
			layoutDirection: isOwn ? Qt.RightToLeft : Qt.LeftToRight
			spacing: 0

			Avatar {
				id: avatar
				visible: !isOwn && isGroupBegin
				jid: root.senderId
				name: root.senderName
				Layout.preferredHeight: Kirigami.Units.gridUnit * 2
				Layout.alignment: Qt.AlignTop
			}

			// message bubble
			Controls.Control {
				id: bubble

				readonly property string paddingText: {
					Utils.messageBubblepaddingCharacter.repeat(Math.ceil(background.metaInfoWidth / background.dummy.implicitWidth))
				}

				readonly property alias backgroundColor: bubbleBackground.color

				topPadding: Kirigami.Units.largeSpacing
				bottomPadding: Kirigami.Units.largeSpacing
				leftPadding: Kirigami.Units.largeSpacing + background.tailSize
				rightPadding: Kirigami.Units.largeSpacing
				Layout.leftMargin: isOwn || avatar.visible ? 0 : avatar.width

				background: MessageBackground {
					id: bubbleBackground
					message: root
					showTail: !isOwn && isGroupBegin

					MouseArea {
						anchors.fill: parent
						acceptedButtons: Qt.LeftButton | Qt.RightButton

						onClicked: {
							if (mouse.button === Qt.RightButton)
								showContextMenu()
						}

						onPressAndHold: showContextMenu()
					}
				}

				contentItem: ColumnLayout {
					Controls.Label {
						text: root.senderName
						color: Utils.userColor(root.senderId, root.senderName)
						font.weight: Font.Medium
						visible: root.isGroupChatMessage && !root.isOwn && root.isGroupBegin && text.length
					}

					// spoiler hint area
					ColumnLayout {
						visible: isSpoiler
						Layout.minimumWidth: bubbleBackground.metaInfoWidth
						Layout.bottomMargin: isShowingSpoiler ? 0 : Kirigami.Units.largeSpacing * 2

						RowLayout {
							FormattedTextEdit {
								text: root.spoilerHint ? root.spoilerHint : qsTr("Spoiler")
								enabled: true
								enhancedFormatting: true
								Layout.fillWidth: true
							}

							ClickableIcon {
								source: isShowingSpoiler ? "password-show-off" : "password-show-on"
								Layout.leftMargin: Kirigami.Units.largeSpacing
								onClicked: isShowingSpoiler = !isShowingSpoiler
							}
						}

						Kirigami.Separator {
							visible: isShowingSpoiler
							Layout.fillWidth: true
							color: {
								const bgColor = Kirigami.Theme.backgroundColor
								const textColor = Kirigami.Theme.textColor
								return Qt.tint(textColor, Qt.rgba(bgColor.r, bgColor.g, bgColor.b, 0.7))
							}
						}
					}

					ColumnLayout {
						visible: isSpoiler && isShowingSpoiler || !isSpoiler

						Repeater {
							model: root.files

							Layout.preferredWidth: 200
							Layout.preferredHeight: 200

							delegate: MediaPreviewOther {
								required property var modelData

								messageId: root.msgId

								mediaSource: {
									if (modelData.localFilePath) {
										let local = MediaUtilsInstance.fromLocalFile(modelData.localFilePath);
										if (MediaUtilsInstance.localFileAvailable(local)) {
											return local;
										}
									}
									return "";
								}
								message: root
								file: modelData
							}
						}

						Loader {
							sourceComponent: {
								if (root.groupChatInvitationJid) {
									return groupChatInvitationComponent
								}

								if (root.messageBody) {
									return bodyComponent
								}

								return undefined
							}

							Component {
								id: bodyComponent

								RowLayout {
									FormattedTextEdit {
										text: root.messageBody + bubble.paddingText
										enabled: true
										visible: messageBody
										enhancedFormatting: true
										Layout.maximumWidth: root.width - Kirigami.Units.gridUnit * 6
									}
								}
							}

							Component {
								id: groupChatInvitationComponent

								RowLayout {
									id: groupChatInvitationArea

									ClickableIcon {
										id: groupChatInvitationButton
										source: "resource-group"
										width: Kirigami.Units.iconSizes.small
										mouseArea.anchors.topMargin: - Kirigami.Units.smallSpacing * 2
										mouseArea.anchors.leftMargin: - Kirigami.Units.smallSpacing * 2
										mouseArea.anchors.rightMargin: - Kirigami.Units.smallSpacing * 1.3
										mouseArea.anchors.bottomMargin: - Kirigami.Units.smallSpacing * 2
										Controls.ToolTip.text: groupChatWatcher.item.isGroupChat ? qsTr("Open") : qsTr("Join…")
										Layout.fillHeight: true
										onClicked: {
											if (groupChatWatcher.item.isGroupChat) {
												Kaidan.openChatPageRequested(ChatController.accountJid, root.groupChatInvitationJid)
											} else {
												openView(groupChatJoiningDialog, groupChatJoiningPage).groupChatJid = root.groupChatInvitationJid
											}
										}
									}

									Kirigami.Separator {
										id: groupChatInvitationSeparator
										implicitWidth: 3
										color: {
											const accentColor = bubble.backgroundColor
											Qt.tint(secondaryBackgroundColor, Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.1))
										}
										Layout.topMargin: - Kirigami.Units.smallSpacing * 2
										Layout.bottomMargin: - Kirigami.Units.smallSpacing * 2
										Layout.rightMargin: Kirigami.Units.largeSpacing
										Layout.fillHeight: true
									}

									Controls.Label {
										text: root.messageBody + bubble.paddingText
										wrapMode: Text.Wrap
										Layout.maximumWidth: {
											return root.width - Kirigami.Units.gridUnit * 6
												- groupChatInvitationButton.width
												- groupChatInvitationSeparator.implicitWidth
												- groupChatInvitationSeparator.Layout.rightMargin
												- groupChatInvitationArea.spacing * 2
										}
									}

									RosterItemWatcher {
										id: groupChatWatcher
										jid: root.groupChatInvitationJid
									}
								}
							}
						}
					}

					// message reactions (emojis in reaction to this message)
					Flow {
						visible: displayedReactionsArea.count
						spacing: Kirigami.Units.smallSpacing
						Layout.bottomMargin: Kirigami.Units.smallSpacing * 5
						Layout.maximumWidth: root.width - Kirigami.Units.gridUnit * 6
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
									accentColor: bubble.backgroundColor
									messageId: root.msgId
									isOwnMessage: root.isOwn
									reactions: root.detailedReactions
									detailsSheet: root.reactionDetailsSheet
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
								}
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
			visible: root.isLastReadContactMessage && ChatController.accountJid !== ChatController.chatJid
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
			visible: root.isLastReadOwnMessage && ChatController.accountJid !== ChatController.chatJid
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
	}

	/**
	 * Shows a context menu (if available) for this message.
	 *
	 * That is especially the case when this message is an element of the ChatPage.
	 */
	function showContextMenu() {
		if (contextMenu) {
			contextMenu.file = null
			contextMenu.message = this
			contextMenu.popup()
		}
	}
}
