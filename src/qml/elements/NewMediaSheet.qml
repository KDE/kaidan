// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0
import MediaUtils 0.1

Kirigami.OverlaySheet {
	id: root

	property int sourceType
	property string source
	property MessageComposition composition

	signal rejected()
	signal accepted()

	showCloseButton: false

	contentItem: ColumnLayout {
		// message type preview
		Loader {
			id: loader

			enabled: true
			visible: enabled
			sourceComponent: newMediaComponent

			Layout.fillHeight: item ? item.Layout.fillHeight : false
			Layout.fillWidth: item ? item.Layout.fillWidth : false
			Layout.preferredHeight: item ? item.Layout.preferredHeight : -1
			Layout.preferredWidth: item ? item.Layout.preferredWidth : -1
			Layout.minimumHeight: item ? item.Layout.minimumHeight : -1
			Layout.minimumWidth: item ? item.Layout.minimumWidth : -1
			Layout.maximumHeight: item ? item.Layout.maximumHeight : -1
			Layout.maximumWidth: item ? item.Layout.maximumWidth : -1
			Layout.alignment: item ? item.Layout.alignment : Qt.AlignCenter
			Layout.margins: item ? item.Layout.margins : 0
			Layout.leftMargin: item ? item.Layout.leftMargin : 0
			Layout.topMargin: item ? item.Layout.topMargin : 0
			Layout.rightMargin: item ? item.Layout.rightMargin : 0
			Layout.bottomMargin: item ? item.Layout.bottomMargin : 0

			Component {
				id: newMediaComponent

				NewMediaLoader {
					mediaSourceType: root.sourceType
					mediaSheet: root
				}
			}

			Component {
				id: mediaPreviewComponent

				MediaPreviewLoader {
					mediaSource: root.source
					mediaSourceType: root.sourceType
					mediaSheet: root
				}
			}
		}

		// buttons for send/cancel
		RowLayout {
			Layout.topMargin: Kirigami.Units.largeSpacing
			Layout.fillWidth: true

			Button {
				text: qsTr("Cancel")

				Layout.fillWidth: true

				onClicked: {
					close()
					root.rejected()
				}
			}

			Button {
				id: sendButton

				enabled: root.source
				text: qsTr("Send")

				Layout.fillWidth: true

				onClicked: {
					composition.fileSelectionModel.addFile(root.source)
					composition.send()
					close()
					root.accepted()
				}
			}
		}

		Keys.onPressed: {
			if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
				sendButton.clicked()
			}
		}
	}

	onSheetOpenChanged: {
		if (!sheetOpen) {
			sourceType = Enums.MessageType.MessageUnknown
		}
	}

	function sendMessageType(jid, type) {
		sourceType = type
		open()
	}

	function sendNewMessageType(jid, type) {
		sendMessageType(jid, type)
	}

	function sendFile(jid, url) {
		source = url
		sendMessageType(jid, MediaUtilsInstance.messageType(url))
	}
}
