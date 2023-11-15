// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Mathis Brüchert <mbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Bhavy Airi <airiraghav@gmail.com>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0
import MediaUtils 0.1

/**
 * This is a pane for writing and sending chat messages.
 */
Controls.Pane {
	id: root
	padding: 6

	background: Kirigami.ShadowedRectangle {
		shadow.color: Qt.darker(color, 1.2)
		shadow.size: 4
		color: Kirigami.Theme.backgroundColor
	}

	property QtObject chatPage
	property alias messageArea: messageArea
	property string messageToCorrect
	property int lastMessageLength: 0
	property MessageComposition composition: MessageComposition {
		accountJid: MessageModel.currentAccountJid
		chatJid: MessageModel.currentChatJid
		replaceId: messageToCorrect
		body: messageArea.text
		spoilerHint: spoilerHintField.text

		onIsDraftChanged: {
			if (isDraft) {
				if (replaceId) {
					prepareMessageCorrection(replaceId, body, spoilerHint)
				} else {
					messageArea.text = body
					spoilerHintField.text = spoilerHint

					// Position the cursor after the draft message's body.
					messageArea.cursorPosition = messageArea.text.length
				}
			}
		}
	}

	ColumnLayout {
		anchors.fill: parent
		spacing: 0

		RowLayout {
			visible: composition.isSpoiler
			spacing: 0

			Controls.TextArea {
				id: spoilerHintField
				Layout.fillWidth: true
				placeholderText: qsTr("Visible message part")
				wrapMode: Controls.TextArea.Wrap
				selectByMouse: true
				background: Item {}
			}

			Controls.Button {
				text: qsTr("Cancel adding hidden message part")
				icon.name: "window-close-symbolic"
				display: Controls.Button.IconOnly
				flat: true

				onClicked: {
					composition.isSpoiler = false
					spoilerHintField.text = ""
				}
			}
		}

		Kirigami.Separator {
			visible: composition.isSpoiler
			Layout.fillWidth: true
			Layout.topMargin: root.padding
			Layout.bottomMargin: Layout.topMargin
		}

		RowLayout {
			spacing: 0

			// emoji picker button
			ClickableIcon {
				source: "face-smile"
				enabled: sendButton.enabled
				onClicked: !emojiPicker.toggle()
			}

			EmojiPicker {
				id: emojiPicker
				x: - root.padding
				y: - height - root.padding
				textArea: messageArea
			}

			Controls.TextArea {
				id: messageArea
				placeholderText: {
					if (root.composition.isSpoiler) {
						return MessageModel.isOmemoEncryptionEnabled ? qsTr("Compose <b>encrypted </b> message with hidden part") : qsTr("Compose <b>unencrypted</b> message with hidden part")
					} else {
						return MessageModel.isOmemoEncryptionEnabled ? qsTr("Compose <b>encrypted</b> message") : qsTr("Compose <b>unencrypted</b> message")
					}
				}
				background: Item {}
				wrapMode: TextEdit.Wrap
				Layout.leftMargin: Style.isMaterial ? 6 : 0
				Layout.rightMargin: Style.isMaterial ? 6 : 0
				Layout.bottomMargin: Style.isMaterial ? -8 : 0
				Layout.fillWidth: true
				verticalAlignment: TextEdit.AlignVCenter
				state: "compose"

				onTextChanged: {
					handleShortcuts()

					// Skip events in which the text field was emptied (probably automatically after sending)
					if (text) {
						MessageModel.sendChatState(ChatState.Composing)
					} else {
						MessageModel.sendChatState(ChatState.Active)
					}
				}

				states: [
					State {
						name: "compose"
					},

					State {
						name: "edit"
					}
				]

				Keys.onReturnPressed: {
					if (event.key === Qt.Key_Return) {
						if (event.modifiers & (Qt.ControlModifier | Qt.ShiftModifier)) {
							messageArea.append("")
						} else {
							sendMessage()
							event.accepted = true
						}
					}
				}

				Connections {
					target: chatPage.searchBar

					// Restore the active focus when searchBar is closed.
					function onActiveChanged() {
						if (!chatPage.searchBar.active) {
							root.forceActiveFocus()
						}
					}
				}

				Connections {
					target: chatPage.messageReactionEmojiPicker

					// Restore the active focus when messageReactionEmojiPicker is closed.
					function onClosed() {
						root.forceActiveFocus()
					}
				}
			}

			// Voice message button
			ClickableIcon {
				source: MediaUtilsInstance.newMediaIconName(Enums.MessageType.MessageAudio)
				visible: messageArea.text === ""

				opacity: visible ? 1 : 0
				Behavior on opacity {
					NumberAnimation {}
				}

				onClicked: {
					chatPage.newMediaSheet.sendNewMessageType(MessageModel.currentChatJid, Enums.MessageType.MessageAudio)
				}
			}

			// file sharing button
			ClickableIcon {
				source: "mail-attachment-symbolic"
				visible: Kaidan.serverFeaturesCache.httpUploadSupported && messageArea.text === ""
				opacity: visible ? 1 : 0
				Behavior on opacity {
					NumberAnimation {}
				}

				property bool checked: false
				onClicked: {
					if (!checked) {
						mediaPopup.open()
						checked = true
					} else {
						mediaPopup.close()
						checked = false
					}
				}
			}

			Controls.Popup {
				id: mediaPopup
				x:  root.width - width - 40
				y: - height - root.padding - 20
				padding: 1

				width: 470
				ColumnLayout {
					anchors.fill: parent
					Kirigami.AbstractApplicationHeader {
						Layout.fillWidth: true
						leftPadding: Kirigami.Units.largeSpacing
						Kirigami.Heading {
							text: qsTr("Media")
						}

					}
					Controls.ScrollView {
						Layout.fillWidth: true
						Layout.fillHeight: true

						visible: thumbnails.count !== 0
						clip: true

						RowLayout {
							Repeater {
								id: thumbnails
								Layout.fillHeight: true
								Layout.fillWidth: true
								model: RecentPicturesModel {}

								delegate: Item {
									Layout.margins: Kirigami.Units.smallSpacing

									Layout.fillWidth: true
									Layout.preferredHeight: 125
									Layout.preferredWidth: 150

									MouseArea {
										anchors.fill: parent
										onClicked: {
											chatPage.sendMediaSheet.openWithExistingFile(model.filePath)
											mediaPopup.close()
										}
									}

									Image {
										source: model.filePath
										height: 125
										width: 150
										sourceSize: "125x150"
										fillMode: Image.PreserveAspectFit
										asynchronous: true
									}
								}
							}
						}
					}

					RowLayout {
						Layout.fillWidth: true
						RowLayout {
							Layout.margins: 5
							Layout.fillWidth: true

							IconTopButton {
								Layout.fillWidth: true
								implicitWidth: parent.width / 4
								buttonIcon: "camera-photo-symbolic"
								title: qsTr("Take picture")
								tooltipText: qsTr("Take a picture using your camera")

								onClicked: {
									chatPage.newMediaSheet.sendNewMessageType(MessageModel.currentChatJid, Enums.MessageType.MessageImage)
									mediaPopup.close()
								}
							}
							IconTopButton {
								Layout.fillWidth: true
								implicitWidth: parent.width / 4
								buttonIcon: "camera-video-symbolic"
								title: qsTr("Record video")
								tooltipText: qsTr("Record a video using your camera")

								onClicked: {
									chatPage.newMediaSheet.sendNewMessageType(MessageModel.currentChatJid, Enums.MessageType.MessageVideo)
									mediaPopup.close()
								}
							}
							IconTopButton {
								Layout.fillWidth: true
								implicitWidth: parent.width / 4
								buttonIcon: "document-open-symbolic"
								title: qsTr("Share files")
								tooltipText: qsTr("Share files from your device")

								onClicked: {
									chatPage.sendMediaSheet.selectFile()
									mediaPopup.close()
								}
							}
							IconTopButton {
								Layout.fillWidth: true
								implicitWidth: parent.width / 4
								buttonIcon: "mark-location-symbolic"
								title: qsTr("Share location")
								tooltipText: qsTr("Send your location")

								onClicked: {
									chatPage.newMediaSheet.sendNewMessageType(MessageModel.currentChatJid, Enums.MessageType.MessageGeoLocation)
									mediaPopup.close()
								}
							}
						}
					}
				}
			}
			ClickableIcon {
				id: sendButton
				visible: messageArea.text !== ""
				opacity: visible ? 1 : 0
				Behavior on opacity {
					NumberAnimation {}
				}
				source: {
					if (messageArea.state === "compose")
						return "mail-send-symbolic"
					else if (messageArea.state === "edit")
						return "document-edit-symbolic"
				}

				onClicked: sendMessage()
			}
		}
	}

	/**
	 * Forces the active focus on desktop devices.
	 *
	 * The focus is not forced on mobile devices because the soft keyboard would otherwise pop up.
	 */
	function forceActiveFocus() {
		if (!Kirigami.Settings.isMobile) {
			messageArea.forceActiveFocus()
		}
	}

	function prepareMessageCorrection(replaceId, body, spoilerHint) {
		messageToCorrect = replaceId
		messageArea.text = body
		composition.isSpoiler = spoilerHint.length
		spoilerHintField.text = spoilerHint
		messageArea.state = "edit"

		// Move the cursor to the end of the text being corrected and focus it.
		messageArea.cursorPosition = messageArea.text.length
		forceActiveFocus()
	}

	/**
	 * Sends the text entered in the messageArea.
	 */
	function sendMessage() {
		// Do not send empty messages.
		if (!messageArea.text.length)
			return

		// Disable the button to prevent sending the same message several times.
		sendButton.enabled = false

		// Send the message.
		if (messageArea.state === "compose") {
			composition.send()
		} else if (messageArea.state === "edit") {
			MessageModel.correctMessage(messageToCorrect, messageArea.text, spoilerHintField.text)
			composition.isDraft = false
		}
		MessageModel.resetComposingChatState();

		clearMessageArea()

		// Enable the button again.
		sendButton.enabled = true

		// Show the cursor even if another element like the sendButton (after
		// clicking on it) was focused before.
		messageArea.forceActiveFocus()
	}

	/**
	 * Handles characters used for special actions.
	 */
	function handleShortcuts() {
		const currentCharacter = messageArea.getText(messageArea.cursorPosition - 1, messageArea.cursorPosition)

		if (emojiPicker.isSearchActive()) {
			if (emojiPicker.searchedText === "" || currentCharacter === "" || currentCharacter === " ") {
				emojiPicker.close()
				return
			}

			// Handle the deletion or addition of characters.
			if (lastMessageLength >= messageArea.text.length)
				emojiPicker.searchedText = emojiPicker.searchedText.substr(0, emojiPicker.searchedText.length - 1)
			else
				emojiPicker.searchedText += currentCharacter

			emojiPicker.search()
		} else {
			if (currentCharacter === ":") {
				if (messageArea.cursorPosition !== 1) {
					const predecessorOfCurrentCharacter = messageArea.getText(messageArea.cursorPosition - 2, messageArea.cursorPosition - 1)
					if (predecessorOfCurrentCharacter === " " || predecessorOfCurrentCharacter === "\n") {
						emojiPicker.openForSearch(currentCharacter)
						emojiPicker.search()
					}
				} else {
					emojiPicker.openForSearch(currentCharacter)
					emojiPicker.search()
				}
			}
		}

		lastMessageLength = messageArea.text.length
	}

	function clearMessageArea() {
		messageArea.text = ""
		composition.isSpoiler = false
		spoilerHintField.text = ""
		messageToCorrect = ""
		messageArea.state = "compose"
	}
}
