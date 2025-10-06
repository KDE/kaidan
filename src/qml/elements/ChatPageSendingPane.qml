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
	property Dialog participantPicker
	property int lastMessageLength: 0
	property real horizontalContentMargin: Kirigami.Units.smallSpacing
	property MessageComposition composition: MessageComposition {
		chatController: root.chatPage.chatController
		body: messageArea.text
		spoilerHint: spoilerHintArea.text
		onIsDraftChanged: {
			if (isDraft) {
				if (replaceId) {
					prepareUiForCorrection(replaceId, replyToJid, replyToGroupChatParticipantId, replyToName, replyId, replyQuote, body, spoilerHint)
				} else {
					messageArea.text = body
					spoilerHintArea.text = spoilerHint

					// Position the cursor after the draft message's body.
					messageArea.cursorPosition = messageArea.text.length
				}
			}
		}
		onPreparedForNewChat: {
			if (isForwarding) {
				messageArea.text = body

				// Position the cursor after the forwarded message's body.
				messageArea.cursorPosition = messageArea.text.length
			}

			if (!isDraft && !isForwarding) {
				root.clear()
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
			account: root.chatPage.chatController.account
			senderId: root.composition.replyToJid ? root.composition.replyToJid : root.composition.replyToGroupChatParticipantId
			senderName: root.composition.replyToName
			messageId: root.composition.replyId
			body: root.composition.replyQuote
			messageListView: root.chatPage.messageListView
			minimumWidth: root.width - root.leftPadding - spacing - replyCancelingButton.width - root.horizontalContentMargin - root.rightPadding
			maximumWidth: minimumWidth
			backgroundRadius: root.background.radius
			quoteBarVisible: false
			visible: root.composition.replyId
			Layout.bottomMargin: Kirigami.Units.largeSpacing

			ClickableIcon {
				id: replyCancelingButton
				Controls.ToolTip.text: qsTr("Cancel reply")
				source: "window-close-symbolic"
				onClicked: {
					root.composition.replyToJid = ""
					root.composition.replyToGroupChatParticipantId = ""
					root.composition.replyToName = ""
					root.composition.replyId = ""
					root.composition.replyQuote = ""
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
				minimumWidth: root.width - root.leftPadding - root.horizontalContentMargin - root.rightPadding - (ListView.view.Controls.ScrollBar.vertical.visible ? ListView.view.Controls.ScrollBar.vertical.width + Kirigami.Units.largeSpacing : 0)
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
			visible: root.composition.isSpoiler
			spacing: 0
			Layout.leftMargin: root.horizontalContentMargin
			Layout.rightMargin: root.horizontalContentMargin

			FormattedTextArea {
				id: spoilerHintArea
				placeholderText: qsTr("Visible message part")
				selectByMouse: true
				background: null
				Layout.fillWidth: true
			}

			ClickableIcon {
				Controls.ToolTip.text: qsTr("Cancel adding hidden message part")
				source: "window-close-symbolic"
				onClicked: {
					root.composition.isSpoiler = false
					spoilerHintArea.clear()
					root.forceActiveFocus()
				}
			}
		}

		Kirigami.Separator {
			visible: root.composition.isSpoiler
			Layout.fillWidth: true
			Layout.margins: root.horizontalContentMargin
		}

		RowLayout {
			spacing: Kirigami.Units.largeSpacing
			Layout.leftMargin: root.horizontalContentMargin
			Layout.rightMargin: root.horizontalContentMargin

			// emoji picker button
			ClickableIcon {
				id: emojiButton
				source:  "emoji-people-symbolic"
				fallback: "smiley-symbolic"
				enabled: voiceMessageRecorder.recorderState !== MediaRecorder.RecordingState
				Controls.ToolTip.text: qsTr("Add an emoji")
				onClicked: !emojiPicker.toggle()
			}

			// group chat participant mentioning button
			ClickableIcon {
				source: "avatar-default-symbolic"
				visible: root.chatPage.chatController.rosterItem.isGroupChat
				enabled: voiceMessageRecorder.recorderState !== MediaRecorder.RecordingState
				opacity: visible ? 1 : 0
				Controls.ToolTip.text: qsTr("Mention a participant")
				onClicked: {
					messageArea.selectWord()
					messageArea.select(messageArea.selectionStart - 1, messageArea.selectionEnd)

					const selectedWord = messageArea.selectedText
					const mentionPrefix = Utils.groupChatUserMentionPrefix

					if (selectedWord.startsWith(mentionPrefix)) {
						messageArea.remove(messageArea.selectionStart, messageArea.selectionEnd)
					} else {
						messageArea.deselect()
						const cursorPosition = messageArea.cursorPosition
						const mentionSeparator = Utils.groupChatUserMentionSeparator

						if (cursorPosition === 0 || selectedWord === mentionSeparator || selectedWord === "\n") {
							messageArea.insert(cursorPosition, mentionPrefix)
						} else {
							messageArea.insert(cursorPosition, mentionSeparator + mentionPrefix)
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
						return root.chatPage.chatController.isEncryptionEnabled ? qsTr("Compose <b>encrypted </b> message with hidden part") : qsTr("Compose <b>unencrypted</b> message with hidden part")
					} else {
						return root.chatPage.chatController.isEncryptionEnabled ? qsTr("Compose <b>encrypted</b> message") : qsTr("Compose <b>unencrypted</b> message")
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

					// Remove text solely consisting of whitespace.
					// That forbids sending messages without any visible characters.
					if (text && !text.trim()) {
						messageArea.clear()
					}

					// Skip events in which the text field was emptied (probably automatically after sending)
					if (text) {
						root.chatPage.chatController.sendChatState(ChatState.Composing)
					} else {
						root.chatPage.chatController.sendChatState(ChatState.Active)
					}
				}
				Keys.onDownPressed: event => {
					if (participantPicker) {
						if (participantPicker.listView.currentIndex === participantPicker.listView.count - 1) {
							participantPicker.listView.currentIndex = 0
						} else {
							participantPicker.listView.incrementCurrentIndex()
						}
					} else {
						event.accepted = false
					}
				}
				Keys.onUpPressed: event => {
					if (participantPicker) {
						if (participantPicker.listView.currentIndex === 0) {
							participantPicker.listView.currentIndex = participantPicker.listView.count - 1
						} else {
							participantPicker.listView.decrementCurrentIndex()
						}
					} else {
						event.accepted = false
					}
				}
				Keys.onLeftPressed: event => {
					if (participantPicker) {
						event.accepted = true
					} else {
						event.accepted = false
					}
				}
				Keys.onRightPressed: event => {
					if (participantPicker) {
						event.accepted = true
					} else {
						event.accepted = false
					}
				}
				Keys.onReturnPressed: event => {
					if (event.key === Qt.Key_Return) {
						if (event.modifiers & (Qt.ControlModifier | Qt.ShiftModifier)) {
							messageArea.append("")
						} else {
							if (participantPicker) {
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
						if (!root.chatPage.searchBar.active) {
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

				function mentionParticipant(mention) {
					insert(cursorPosition, mention + Utils.groupChatUserMentionSeparator)
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
					visible: root.chatPage.chatController.account.settings.httpUploadLimit
					onClicked: expansionArea.openDialog(imageCaptureDialog)
				}

				ClickableIcon {
					Controls.ToolTip.text: qsTr("Record a video")
					source: "camera-video-symbolic"
					visible: root.chatPage.chatController.account.settings.httpUploadLimit
					onClicked: expansionArea.openDialog(videoRecordingDialog)
				}

				ClickableIcon {
					Controls.ToolTip.text: qsTr("Share files")
					source: "folder-symbolic"
					visible: root.chatPage.chatController.account.settings.httpUploadLimit
					onClicked: {
						expansionArea.visible = false
						root.composition.fileSelectionModel.selectFile()
					}
				}

				ClickableIcon {
					Controls.ToolTip.text: qsTr("Share your location")
					source: "mark-location-symbolic"
					visible: root.chatPage.chatController.account.connection.state === Enums.StateConnected
					onClicked: expansionArea.openDialog(geoLocationSharingDialog)
				}

				ClickableIcon {
					Controls.ToolTip.text: qsTr("Add hidden message part")
					source: "eye-not-looking-symbolic"
					fallback: "password-show-off"
					visible: !root.composition.isSpoiler
					onClicked: {
						expansionArea.visible = false
						root.composition.isSpoiler = true
						spoilerHintArea.forceActiveFocus()
					}
				}

				Component {
					id: imageCaptureDialog

					ImageCaptureDialog {
						fileSelectionModel: root.composition.fileSelectionModel
					}
				}

				Component {
					id: videoRecordingDialog

					VideoRecordingDialog {
						fileSelectionModel: root.composition.fileSelectionModel
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
				visible: voiceMessageCaptureSession.audioInput && root.chatPage.chatController.account.settings.httpUploadLimit && root.chatPage.chatController.account.connection.state === Enums.StateConnected && !root.composition.body && !root.composition.replaceId
				opacity: visible ? 1 : 0
				onClicked: {
					if (voiceMessageRecorder.recorderState === MediaRecorder.RecordingState) {
						voiceMessageRecorder.stop()
					} else {
						voiceMessageRecorder.outputLocation = MediaUtils.newAudioFileUrl()
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
				visible: (mediaList.count && voiceMessageRecorder.recorderState !== MediaRecorder.RecordingState && root.chatPage.chatController.account.connection.state === Enums.StateConnected) || (root.composition.body && (!root.composition.replaceId || root.composition.body !== root.composition.originalBody || root.composition.replyId !== root.composition.originalReplyId))
				opacity: visible ? 1 : 0
				Controls.ToolTip.text: qsTr("Send")
				onClicked: {
					// Do not allow sending via keys if hidden.
					if (!visible) {
						return
					}

					// Disable the button to prevent sending the same message multiple times.
					enabled = false

					// Send the message.
					if (root.composition.replaceId) {
						root.composition.correct()
					} else {
						root.composition.send()
					}

					root.chatPage.chatController.resetComposingChatState();
					clear()

					root.chatPage.messageListView.positionViewAtLatestMessage()

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
				onClicked: {
					if (voiceMessageRecorder.recorderState === MediaRecorder.RecordingState) {
						voiceMessageRecorder.recordingCanceled = true
						voiceMessageRecorder.stop()
					} else {
						root.composition.clear()
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

			Component {
				id: participantPickerComponent

				GroupChatParticipantPicker {
					x: {
						const cursorRectangle = messageArea.cursorRectangle

						// A gap is needed since cursorRectangle.x is (for an unknown reason) not at the cursor's position.
						const gap = Kirigami.Units.largeSpacing * 8

						if (root.chatPage.width > cursorRectangle.x + gap + listView.implicitWidth) {
							return mapToGlobal(cursorRectangle.x, cursorRectangle.y).x + gap
						}

						return pageStack.wideMode ? root.chatPage.x : 0
					}
					y: {
						// Used to trigger a reevaluation of y if the window's height changes.
						const windowHeight = applicationWindow().height

						const cursorRectangle = messageArea.cursorRectangle
						return mapToGlobal(cursorRectangle.x, cursorRectangle.y).y - contentHeight
					}
					account: root.chatPage.chatController.account
					chatJid: root.chatPage.chatController.jid
					textArea: messageArea
				}
			}

			CaptureSession {
				id: voiceMessageCaptureSession
				audioInput: AudioInput {}
				recorder: MediaRecorder {
					id: voiceMessageRecorder

					property bool recordingCanceled: false

					onRecorderStateChanged: {
						if (recorderState === MediaRecorder.StoppedState) {
							// Delete or send the recorded voice message once its data is completely
							// processed.
							if (recordingCanceled) {
								recordingCanceled = false
								MediaUtils.deleteFile(actualLocation)
							} else {
								root.composition.fileSelectionModel.addFile(actualLocation)
								root.composition.send()
							}
						}
					}
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
		composition.originalBody = body
		composition.isSpoiler = spoilerHint.length

		prepareReply(replyToJid, replyToGroupChatParticipantId, replyToName, replyId, replyQuote)
		prepareUiForCorrection(replaceId, replyToJid, replyToGroupChatParticipantId, replyToName, replyId, replyQuote, body, spoilerHint)
	}

	function prepareUiForCorrection(replaceId, replyToJid, replyToGroupChatParticipantId, replyToName, replyId, replyQuote, body, spoilerHint) {
		messageArea.text = body
		spoilerHintArea.text = spoilerHint

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
			if (lastMessageLength >= messageArea.text.length) {
				emojiPicker.searchedText = emojiPicker.searchedText.substr(0, emojiPicker.searchedText.length - 1)
			} else {
				emojiPicker.searchedText += currentCharacter
			}

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

		if (participantPicker) {
			const searchedText = participantPicker.searchedText

			if (currentCharacter === "" || currentCharacter === " " || currentCharacter === "\n") {
				participantPicker.close()
				return
			}

			// Handle the deletion or addition of characters.
			if (lastMessageLength >= messageArea.text.length) {
				participantPicker.search(searchedText.substr(0, searchedText.length - 1))
			} else {
				participantPicker.search(searchedText + currentCharacter)
			}
		} else if (root.chatPage.chatController.rosterItem.isGroupChat && currentCharacter === Utils.groupChatUserMentionPrefix) {
			if (messageArea.cursorPosition !== 1) {
				const predecessorOfCurrentCharacter = messageArea.getText(messageArea.cursorPosition - 2, messageArea.cursorPosition - 1)
				if (predecessorOfCurrentCharacter === " " || predecessorOfCurrentCharacter === "\n") {
					openParticipantPicker()
				}
			} else {
				openParticipantPicker()
			}
		}

		lastMessageLength = messageArea.text.length
	}

	function openParticipantPicker() {
		participantPicker = openOverlay(participantPickerComponent)
	}

	function clear() {
		messageArea.clear()
		spoilerHintArea.clear()
		expansionArea.visible = false
	}
}
