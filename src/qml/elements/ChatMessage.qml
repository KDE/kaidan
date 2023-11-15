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

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls
import QtGraphicalEffects 1.14
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
	property int encryption
	property bool isTrusted
	property string messageBody
	property string date
	property string time
	property int deliveryState
	property string deliveryStateName
	property url deliveryStateIcon
	property bool isLastRead
	property bool edited
	property bool isSpoiler
	property string spoilerHint
	property bool isShowingSpoiler: false
	property string errorText: ""
	property alias bodyLabel: bodyLabel
	property var files;
	property var displayedReactions
	property var detailedReactions
	property var ownDetailedReactions

	property bool isGroupBegin: {
		return modelIndex < 1 ||
			MessageModel.data(MessageModel.index(modelIndex - 1, 0), MessageModel.SenderId) !== senderId
	}

	signal messageEditRequested(string replaceId, string body, string spoilerHint)
	signal quoteRequested(string body)

	height: messageArea.implicitHeight + (isGroupBegin ? Kirigami.Units.largeSpacing : Kirigami.Units.smallSpacing)
	alwaysVisibleActions: false

	actions: [
		// TODO: Move message to the left when action is displayed and message is too large or
		// display all actions at the bottom / at the top of the message bubble
		Kirigami.Action {
			text: "Add message reaction"
			icon.name: "smiley-add"
			visible: !root.displayedReactions.length
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

			// placeholder
			Item {
				Layout.preferredWidth: 5
			}

			Item {
				visible: !isOwn
				Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
				Layout.preferredHeight: Kirigami.Units.gridUnit * 2.2
				Layout.preferredWidth: Kirigami.Units.gridUnit * 2.2

				Avatar {
					id: avatar
					visible: !isOwn && isGroupBegin
					anchors.fill: parent
					jid: root.senderId
					name: root.senderName
				}
			}

			// message bubble
			Controls.Control {
				id: bubble

				readonly property string paddingText: {
					"⠀".repeat(Math.ceil(background.metaInfoWidth / background.dummy.implicitWidth))
				}

				readonly property alias backgroundColor: bubbleBackground.color

				topPadding: Kirigami.Units.largeSpacing
				bottomPadding: Kirigami.Units.largeSpacing
				leftPadding: Kirigami.Units.largeSpacing + background.tailSize
				rightPadding: Kirigami.Units.largeSpacing

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
					// spoiler hint area
					ColumnLayout {
						visible: isSpoiler
						Layout.minimumWidth: bubbleBackground.metaInfoWidth
						Layout.bottomMargin: isShowingSpoiler ? 0 : Kirigami.Units.largeSpacing * 2

						RowLayout {
							Controls.Label {
								text: spoilerHint == "" ? qsTr("Spoiler") : Utils.formatMessage(spoilerHint)
								textFormat: Text.StyledText
								wrapMode: Text.Wrap
								color: Kirigami.Theme.textColor
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

						// message body
						Controls.Label {
							id: bodyLabel
							visible: messageBody
							text: Utils.formatMessage(messageBody) + bubble.paddingText
							textFormat: Text.StyledText
							wrapMode: Text.Wrap
							color: Kirigami.Theme.textColor
							onLinkActivated: Qt.openUrlExternally(link)
							Layout.maximumWidth: root.width - Kirigami.Units.gridUnit * 6
						}
					}

					// message reactions (emojis in reaction to this message)
					Flow {
						visible: displayedReactionsArea.count
						spacing: 4
						Layout.bottomMargin: 15
						Layout.maximumWidth: bodyLabel.Layout.maximumWidth
						Layout.preferredWidth: {
							var displayedReactionsWidth = 0

							for (var i = 0; i < displayedReactionsArea.count; i++) {
								displayedReactionsWidth += displayedReactionsArea.itemAt(i).width
							}

							return displayedReactionsWidth + (messageReactionAdditionButton.width * 2) + spacing * (displayedReactionsArea.count + 2)
						}

						Repeater {
							id: displayedReactionsArea
							model: root.displayedReactions

							MessageReactionDisplayButton {
								accentColor: bubble.backgroundColor
								ownReactionIncluded: modelData.ownReactionIncluded
								deliveryState: modelData.deliveryState
								isOwnMessage: root.isOwn
								text: modelData.count === 1 ? modelData.emoji : modelData.emoji + " " + modelData.count
								width: smallButtonWidth + (text.length < 3 ? 0 : (text.length - 2) * Kirigami.Theme.defaultFont.pixelSize * 0.6)
								onClicked: {
									if (ownReactionIncluded &&
										deliveryState !== MessageReactionDeliveryState.PendingRemovalAfterSent &&
										deliveryState !== MessageReactionDeliveryState.PendingRemovalAfterDelivered) {
										MessageModel.removeMessageReaction(root.msgId, modelData.emoji)
									} else {
										MessageModel.addMessageReaction(root.msgId, modelData.emoji)
									}
								}
							}
						}

						MessageReactionAdditionButton {
							id: messageReactionAdditionButton
							messageId: root.msgId
							emojiPicker: root.reactionEmojiPicker
							accentColor: bubble.backgroundColor
						}

						MessageReactionDetailsButton {
							messageId: root.msgId
							accentColor: bubble.backgroundColor
							isOwnMessage: root.isOwn
							detailedReactions: root.detailedReactions
							ownDetailedReactions: root.ownDetailedReactions
							detailsSheet: root.reactionDetailsSheet
						}
					}
				}
			}

			// placeholder
			Item {
				Layout.fillWidth: true
			}
		}

		// Read marker text for own message
		RowLayout {
			visible: root.isLastRead && MessageModel.currentAccountJid !== MessageModel.currentChatJid
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
