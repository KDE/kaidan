// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Mathis Brüchert <mbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Bhavy Airi <airiraghav@gmail.com>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import QtMultimedia
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

/**
 * This is a pane for writing and sending chat messages.
 */
Controls.Pane {
	id: root

	property QtObject chatPage
	property alias messageArea: messageArea
	property string originalBody
	property string originalReplyId
	property int lastMessageLength: 0
	property MessageComposition composition: MessageComposition {
		accountJid: ChatController.accountJid
		chatJid: ChatController.chatJid
		body: messageArea.text
		spoilerHint: spoilerHintField.text
		onIsDraftChanged: {
			if (isDraft) {
				if (replaceId) {
					prepareCorrection(replaceId, replyToJid, replyToGroupChatParticipantId, replyToName, replyId, replyQuote, body, spoilerHint)
				} else {
					messageArea.text = body
					spoilerHintField.text = spoilerHint

					// Position the cursor after the draft message's body.
					messageArea.cursorPosition = messageArea.text.length
				}
			}
		}
	}

	bottomInset: Kirigami.Units.largeSpacing
	leftInset: bottomInset
	rightInset: bottomInset
	topPadding: topInset + Kirigami.Units.mediumSpacing
	bottomPadding: bottomInset + Kirigami.Units.mediumSpacing
	leftPadding: leftInset + Kirigami.Units.mediumSpacing
	rightPadding: rightInset + Kirigami.Units.mediumSpacing
	background: Kirigami.ShadowedRectangle {
		shadow.color: Qt.darker(color, 1.2)
		shadow.size: 4
		color: primaryBackgroundColor
		radius: Kirigami.Units.gridUnit * 1.2
	}
	Component.onDestruction: composition.fileSelectionModel.deleteNewFiles()

	ColumnLayout {
		anchors.fill: parent
		spacing: 0

		ReferencedMessage {
			visible: composition.replyId
			senderId: composition.replyToJid ? composition.replyToJid : composition.replyToGroupChatParticipantId
			senderName: composition.replyToName
			messageId: composition.replyId
			body: composition.replyQuote
			messageListView: root.chatPage.messageListView
			minimumWidth: root.width - root.leftPadding - spacing - replyCancelingButton.width - root.rightPadding
			maximumWidth: minimumWidth
			backgroundRadius: root.background.radius
			quoteBarVisible: false
			Layout.bottomMargin: Kirigami.Units.largeSpacing

			ClickableIcon {
				id: replyCancelingButton
				Controls.ToolTip.text: qsTr("Cancel reply")
				source: "window-close-symbolic"
				Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
				onClicked: {
					composition.replyToJid = ""
					composition.replyToGroupChatParticipantId = ""
					composition.replyToName = ""
					composition.replyId = ""
					composition.replyQuote = ""
					root.forceActiveFocus()
				}
			}
		}

		ListView {
			id: mediaList
			model: root.composition.fileSelectionModel
			delegate: MediumSendingPreview {
				selectionModel: mediaList.model
				modelData: model
				minimumWidth: root.width - root.leftPadding - root.rightPadding - (ListView.view.Controls.ScrollBar.vertical.visible ? ListView.view.Controls.ScrollBar.vertical.width + Kirigami.Units.largeSpacing : 0)
				maximumWidth: minimumWidth
				mainAreaBackground.radius: root.background.radius
				previewImage.radius: root.background.radius
			}
			visible: count
			spacing: Kirigami.Units.smallSpacing
			implicitWidth: contentWidth
			implicitHeight: Math.min(contentHeight, applicationWindow().height / 2)
			clip: true
			Controls.ScrollBar.vertical: Controls.ScrollBar {
				snapMode: Controls.ScrollBar.SnapAlways
			}
			Layout.fillWidth: true
			Layout.bottomMargin: Kirigami.Units.largeSpacing
		}

		RowLayout {
			visible: composition.isSpoiler
			spacing: 0

			FormattedTextArea {
				id: spoilerHintField
				placeholderText: qsTr("Visible message part")
				selectByMouse: true
				background: null
				Layout.fillWidth: true
			}

			ClickableIcon {
				Controls.ToolTip.text: qsTr("Cancel adding hidden message part")
				source: "window-close-symbolic"
				Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
				onClicked: {
					composition.isSpoiler = false
					spoilerHintField.clear()
					root.forceActiveFocus()
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
				source: "smiley-add"
				enabled: voiceMessageRecorder.recorderState !== MediaRecorder.RecordingState
				Controls.ToolTip.text: qsTr("Add an emoji")
				onClicked: !emojiPicker.toggle()
			}

			// group chat pariticipant mentioning button
			ClickableText {
				text: "@"
				visible: ChatController.rosterItem.isGroupChat
				enabled: voiceMessageRecorder.recorderState !== MediaRecorder.RecordingState
				opacity: visible ? 1 : 0
				scaleFactor: Kirigami.Units.iconSizes.smallMedium * 0.08
				Controls.ToolTip.text: qsTr("Mention a participant")
				Layout.topMargin: - scaleFactor * 2
				Layout.leftMargin: Kirigami.Units.smallSpacing
				onClicked: {
					messageArea.selectWord()
					messageArea.select(messageArea.selectionStart - 1, messageArea.selectionEnd)

					const mentionPrefix = Utils.groupChatUserMentionPrefix
					const selectedWord = messageArea.selectedText

					if (selectedWord.startsWith(mentionPrefix)) {
						messageArea.remove(messageArea.selectionStart, messageArea.selectionEnd)
					} else {
						messageArea.deselect()
						const cursorPosition = messageArea.cursorPosition

						if (cursorPosition === 0 || selectedWord === Utils.groupChatUserMentionSeparator) {
							messageArea.insert(cursorPosition, mentionPrefix)
						} else {
							messageArea.insert(cursorPosition, Utils.groupChatUserMentionSeparator + mentionPrefix)
						}
					}
				}

				Behavior on opacity {
					NumberAnimation {}
				}
			}

			FormattedTextArea {
				id: messageArea
				placeholderText: {
					if (root.composition.isSpoiler) {
						return ChatController.isEncryptionEnabled ? qsTr("Compose <b>encrypted </b> message with hidden part") : qsTr("Compose <b>unencrypted</b> message with hidden part")
					} else {
						return ChatController.isEncryptionEnabled ? qsTr("Compose <b>encrypted</b> message") : qsTr("Compose <b>unencrypted</b> message")
					}
				}
				background: null
				Layout.leftMargin: Style.isMaterial ? 6 : 0
				Layout.rightMargin: Style.isMaterial ? 6 : 0
				Layout.bottomMargin: Style.isMaterial ? -8 : 0
				Layout.fillWidth: true
				verticalAlignment: TextEdit.AlignVCenter
				enabled: voiceMessageRecorder.recorderState !== MediaRecorder.RecordingState
				onTextChanged: {
					handleShortcuts()

					// Skip events in which the text field was emptied (probably automatically after sending)
					if (text) {
						ChatController.sendChatState(ChatState.Composing)
					} else {
						ChatController.sendChatState(ChatState.Active)
					}
				}
				Keys.onDownPressed: {
					if (participantPicker.visible) {
						if (participantPicker.listView.currentIndex === participantPicker.listView.count - 1) {
							participantPicker.listView.currentIndex = 0
						} else {
							participantPicker.listView.incrementCurrentIndex()
						}
					}
				}
				Keys.onUpPressed: {
					if (participantPicker.visible) {
						if (participantPicker.listView.currentIndex === 0) {
							participantPicker.listView.currentIndex = participantPicker.listView.count - 1
						} else {
							participantPicker.listView.decrementCurrentIndex()
						}
					}
				}
				Keys.onReturnPressed: {
					if (event.key === Qt.Key_Return) {
						if (event.modifiers & (Qt.ControlModifier | Qt.ShiftModifier)) {
							messageArea.append("")
						} else {
							if (participantPicker.visible) {
								participantPicker.selectCurrentIndex()
							} else {
								sendButton.clicked()
							}

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

			RowLayout {
				id: expansionArea
				visible: false
				opacity: visible ? 1 : 0
				spacing: parent.spacing

				Behavior on opacity {
					NumberAnimation {}
				}

				ClickableIcon {
					Controls.ToolTip.text: qsTr("Take a picture")
					source: "camera-photo-symbolic"
					visible: Kaidan.serverFeaturesCache.httpUploadSupported
					Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
					onClicked: expansionArea.openDialog(imageCaptureDialog)
				}

				ClickableIcon {
					Controls.ToolTip.text: qsTr("Record a video")
					source: "camera-video-symbolic"
					visible: Kaidan.serverFeaturesCache.httpUploadSupported
					Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
					onClicked: expansionArea.openDialog(videoRecordingDialog)
				}

				ClickableIcon {
					Controls.ToolTip.text: qsTr("Share files")
					source: "folder-symbolic"
					visible: Kaidan.serverFeaturesCache.httpUploadSupported
					Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
					onClicked: {
						expansionArea.visible = false
						root.composition.fileSelectionModel.selectFile()
					}
				}

				ClickableIcon {
					Controls.ToolTip.text: qsTr("Share your location")
					source: "mark-location-symbolic"
					visible: Kaidan.connectionState === Enums.StateConnected
					Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
					onClicked: expansionArea.openDialog(geoLocationSharingDialog)
				}

				ClickableIcon {
					Controls.ToolTip.text: qsTr("Add hidden message part")
					source: "eye-not-looking-symbolic"
					fallback: "password-show-off"
					visible: !root.composition.isSpoiler
					Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
					onClicked: {
						expansionArea.visible = false
						root.composition.isSpoiler = true
						spoilerHintField.forceActiveFocus()
					}
				}

				Component {
					id: imageCaptureDialog

					ImageCaptureDialog {
						messageComposition: root.composition
					}
				}

				Component {
					id: videoRecordingDialog

					VideoRecordingDialog {
						messageComposition: root.composition
					}
				}

				Component {
					id: geoLocationSharingDialog

					GeoLocationSharingDialog {
						messageComposition: root.composition
					}
				}

				function openDialog(dialog) {
					visible = false
					openOverlay(dialog)
				}
			}

			// Expansion button
			ClickableIcon {
				Controls.ToolTip.text: expansionArea.visible ? qsTr("Hide") :qsTr("More")
				source: expansionArea.visible ? "window-close-symbolic" : "list-add-symbolic"
				visible: !root.composition.replaceId && voiceMessageRecorder.recorderState !== MediaRecorder.RecordingState
				opacity: visible ? 1 : 0
				Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
				onClicked: expansionArea.visible = !expansionArea.visible

				Behavior on opacity {
					NumberAnimation {}
				}
			}

			// Voice message recording indicator
			RecordingIndicator {
				id: voiceMessageRecordingIndicator
				visible: voiceMessageRecorder.recorderState === MediaRecorder.RecordingState
				opacity: visible ? 1 : 0
				duration: voiceMessageRecorder.duration

				Behavior on opacity {
					NumberAnimation {}
				}

				// Reset the displayed duration after finishing/canceling the recording.
				// Otherwise, the duration of the last recorded voice message would be shown before
				// the duration is reset by the new recording.
				Connections {
					target: voiceMessageRecorder

					property bool isReset: false

					function onRecorderStateChanged() {
						if (voiceMessageRecorder.recorderState === MediaRecorder.StoppedState) {
							voiceMessageRecordingIndicator.duration = 0
							isReset = true
						}
					}

					function onDurationChanged() {
						if (isReset) {
							isReset = false
							voiceMessageRecordingIndicator.duration = Qt.binding(() => { return voiceMessageRecorder.duration })
						}
					}
				}
			}

			// Voice message button
			ClickableIcon {
				Controls.ToolTip.text: qsTr("Send a voice message")
				source: voiceMessageRecorder.recorderState === MediaRecorder.RecordingState ? "media-playback-stop-symbolic" : MediaUtils.newMediaIconName(Enums.MessageType.MessageAudio)
				visible: voiceMessageCaptureSession.audioInput && Kaidan.serverFeaturesCache.httpUploadSupported && Kaidan.connectionState === Enums.StateConnected && !root.composition.body && !root.composition.replaceId
				opacity: visible ? 1 : 0
				Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
				onClicked: {
					if (voiceMessageRecorder.recorderState === MediaRecorder.RecordingState) {
						voiceMessageRecorder.stop()
						root.composition.fileSelectionModel.addFile(voiceMessageRecorder.actualLocation)
						root.composition.send()
					} else {
						voiceMessageRecorder.outputLocation = MediaUtils.newAudioFilePath()
						voiceMessageRecorder.record()
						expansionArea.visible = false
					}
				}

				Behavior on opacity {
					NumberAnimation {}
				}
			}

			ClickableIcon {
				id: sendButton
				source: root.composition.replaceId ? "document-edit-symbolic" : "mail-send-symbolic"
				visible: (mediaList.count && voiceMessageRecorder.recorderState !== MediaRecorder.RecordingState && Kaidan.connectionState === Enums.StateConnected) || (root.composition.body && (!root.composition.replaceId || root.composition.body !== root.originalBody || composition.replyId !== root.originalReplyId))
				opacity: visible ? 1 : 0
				Controls.ToolTip.text: qsTr("Send")
				Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
				onClicked: {
					// Do not allow sending via keys if hidden.
					if (!visible) {
						return
					}

					// Disable the button to prevent sending the same message mutliple times.
					enabled = false

					// Send the message.
					if (root.composition.replaceId) {
						root.composition.correct()
					} else {
						root.composition.send()
					}

					ChatController.resetComposingChatState();
					clear()

					// Enable the button again.
					enabled = true

					// Show the cursor even if another element like the sendButton (after clicking
					// on it) was focused before.
					root.forceActiveFocus()
				}

				Behavior on opacity {
					NumberAnimation {}
				}
			}

			// Button to cancel message correction or voice message recording
			ClickableIcon {
				visible: root.composition.replaceId || voiceMessageRecorder.recorderState === MediaRecorder.RecordingState
				opacity: visible ? 1 : 0
				source: "window-close-symbolic"
				Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
				onClicked: {
					if (voiceMessageRecorder.recorderState === MediaRecorder.RecordingState) {
						voiceMessageRecorder.stop()
						MediaUtils.deleteFile(voiceMessageRecorder.actualLocation)
					} else {
						composition.clear()
						root.clear()
					}
				}

				Behavior on opacity {
					NumberAnimation {}
				}
			}

			EmojiPicker {
				id: emojiPicker
				x: - root.padding
				y: - height - root.padding
				textArea: messageArea
			}

			GroupChatParticipantPicker {
				id: participantPicker
				x: root.chatPage.x + root.leftInset
				y: root.chatPage.height - root.height - root.topPadding - contentHeight
				accountJid: ChatController.accountJid
				chatJid: ChatController.chatJid
				textArea: messageArea
			}

			CaptureSession {
				id: voiceMessageCaptureSession
				audioInput: AudioInput {}
				recorder: MediaRecorder {
					id: voiceMessageRecorder
					mediaFormat.fileFormat: MediaFormat.MP3
				}
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

	function prepareQuote(body) {
		messageArea.insert(0, Utils.quote(body))
	}

	function prepareCorrection(replaceId, replyToJid, replyToGroupChatParticipantId, replyToName, replyId, replyQuote, body, spoilerHint) {
		composition.replaceId = replaceId
		originalReplyId = replyId
		prepareReply(replyToJid, replyToGroupChatParticipantId, replyToName, replyId, replyQuote)
		originalBody = body
		messageArea.text = body
		composition.isSpoiler = spoilerHint.length
		spoilerHintField.text = spoilerHint

		// Move the cursor to the end of the text being corrected and focus it.
		messageArea.cursorPosition = messageArea.text.length
		forceActiveFocus()
	}

	function prepareReply(replyToJid, replyToGroupChatParticipantId, replyToName, replyId, replyQuote) {
		composition.replyToJid = replyToJid
		composition.replyToGroupChatParticipantId = replyToGroupChatParticipantId
		composition.replyToName = replyToName
		composition.replyId = replyId
		composition.replyQuote = replyQuote
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

		if (participantPicker.visible) {
			if (participantPicker.searchedText === "" || currentCharacter === "" || currentCharacter === " ") {
				participantPicker.close()
				return
			}

			// Handle the deletion or addition of characters.
			if (lastMessageLength >= messageArea.text.length) {
				participantPicker.searchedText = participantPicker.searchedText.substr(0, participantPicker.searchedText.length - 1)
			} else {
				participantPicker.searchedText += currentCharacter
			}

			participantPicker.search()
		} else if (ChatController.rosterItem.isGroupChat && currentCharacter === Utils.groupChatUserMentionPrefix) {
			if (messageArea.cursorPosition !== 1) {
				const predecessorOfCurrentCharacter = messageArea.getText(messageArea.cursorPosition - 2, messageArea.cursorPosition - 1)
				if (predecessorOfCurrentCharacter === " " || predecessorOfCurrentCharacter === "\n") {
					participantPicker.openForSearch(currentCharacter)
					participantPicker.search()
				}
			} else {
				participantPicker.openForSearch(currentCharacter)
				participantPicker.search()
			}
		}

		lastMessageLength = messageArea.text.length
	}

	function clear() {
		messageArea.clear()
		spoilerHintField.clear()
		originalBody = ""
		expansionArea.visible = false
	}
}
