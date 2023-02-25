/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2023 Kaidan developers and contributors
 *  (see the LICENSE file for a full list of copyright authors)
 *
 *  Kaidan is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  In addition, as a special exception, the author of Kaidan gives
 *  permission to link the code of its release with the OpenSSL
 *  project's "OpenSSL" library (or with modified versions of it that
 *  use the same license as the "OpenSSL" library), and distribute the
 *  linked executables. You must obey the GNU General Public License in
 *  all respects for all of the code used other than "OpenSSL". If you
 *  modify this file, you may extend this exception to your version of
 *  the file, but you are not obligated to do so.  If you do not wish to
 *  do so, delete this exception statement from your version.
 *
 *  Kaidan is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kaidan.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls
import org.kde.kirigami 2.12 as Kirigami

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
	property int lastMessageLength: 0
	readonly property MessageComposition composition: MessageComposition {
		id: composition
		account: AccountManager.jid
		to: MessageModel.currentChatJid
		body: messageArea.text
		spoilerHint: spoilerHintField.text
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
				placeholderText: qsTr("Spoiler hint")
				wrapMode: Controls.TextArea.Wrap
				selectByMouse: true
				background: Item {}
			}

			Controls.Button {
				text: qsTr("Close spoiler hint field")
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
				placeholderText: qsTr("Compose message")
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

				onStateChanged: {
					if (state === "edit") {
						// Move the cursor to the end of the text being corrected.
						selectAll()
						cursorPosition = selectionEnd
					}
				}

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
				height: 300
				ColumnLayout {
					anchors.fill: parent
					Kirigami.AbstractApplicationHeader {
						Layout.fillWidth: true
						leftPadding: Kirigami.Units.largeSpacing
						Kirigami.Heading {
							text: qsTr("Attachments")
						}

					}
					Controls.ScrollView {
						Layout.fillWidth: true
						Layout.fillHeight: true

						RowLayout {
							Repeater {
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

	Component.onCompleted: {
		// This makes it possible on desktop devices to directly enter a message after opening the chat page.
		// It is not used on mobile devices because the soft keyboard would otherwise always pop up after opening the chat page.
		if (!Kirigami.Settings.isMobile)
			messageArea.forceActiveFocus()
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
			MessageModel.correctMessage(chatPage.messageToCorrect, messageArea.text)
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
		var currentCharacter = messageArea.getText(messageArea.cursorPosition - 1, messageArea.cursorPosition)

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
					var predecessorOfCurrentCharacter = messageArea.getText(messageArea.cursorPosition - 2, messageArea.cursorPosition - 1)
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
		spoilerHintField.text = ""
		chatPage.messageToCorrect = ''
		messageArea.state = "compose"
	}
}
