// SPDX-FileCopyrightText: 2018 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
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

	property string targetJid
	required property MessageComposition composition
	required property QtObject chatPage

	signal rejected()
	signal accepted()

	showCloseButton: false

	header: Kirigami.Heading {
		text: qsTr("Share files")
	}

	// First open the file choose to select a file, then open the sheet
	function selectFile() {
		root.composition.fileSelectionModel.selectFile()
	}

	// Open the sheet containing an already known file
	function openWithExistingFile(localPath) {
		root.composition.fileSelectionModel.addFile(localPath)
		root.ensureOpen()
	}

	// Add a known file to the already open sheet
	function addFile(localPath) {
		root.composition.fileSelectionModel.addFile(localPath)
	}

	// Open the sheet if it is not already open
	function ensureOpen() {
		if (!root.sheetOpen) {
			root.open()
		}
	}

	onSheetOpenChanged: {
		if (!sheetOpen) {
			root.composition.fileSelectionModel.clear()
			messageText.text = ""
		}
	}

	Item {
		Connections {
			target: root.composition && root.composition.fileSelectionModel

			function onSelectFileFinished() {
				if (!root.sheetOpen) {
				   root.open()
				}
			}
		}
	}

	ColumnLayout {
		Kirigami.PlaceholderMessage {
			Layout.alignment: Qt.AlignHCenter
			Layout.topMargin: Kirigami.Units.gridUnit * 10
			Layout.bottomMargin: Kirigami.Units.gridUnit * 10
			text: qsTr("Choose files")
			visible: fileList.count === 0
		}

		// List of selected files
		Repeater {
			id: fileList
			model: root.composition.fileSelectionModel

			delegate: Kirigami.AbstractListItem {
				id: delegateRoot

				contentItem: RowLayout {
					// Icon
					Kirigami.Icon {
						Layout.preferredWidth: Kirigami.Units.iconSizes.huge
						Layout.preferredHeight: Kirigami.Units.iconSizes.huge
						source: model.thumbnail
					}

					// spacer
					Item {
					}

					// File name and description
					ColumnLayout {
						Layout.fillWidth: true

						RowLayout {
							Kirigami.Heading {
								Layout.fillWidth: true

								level: 3
								text: model.fileName
							}

							Controls.Label {
								text: model.fileSize
							}
						}

						Controls.TextField {
							Layout.fillWidth: true

							text: model.description
							placeholderText: qsTr("Enter description…")

							onTextChanged: model.description = text
						}
					}

					Controls.ToolButton {
						icon.name: "list-remove-symbolic"
						text: qsTr("Remove file")
						display: Controls.AbstractButton.IconOnly
						onClicked: root.composition.fileSelectionModel.removeFile(model.index)
					}
				}
			}
		}

		Controls.TextField {
			id: messageText

			Layout.fillWidth: true
			Layout.topMargin: Kirigami.Units.largeSpacing

			placeholderText: qsTr("Compose message")
			onFocusChanged: root.composition.body = messageText.text
		}

		// Button row
		RowLayout {
			Controls.ToolButton {
				text: qsTr("Add")
				icon.name: "list-add-symbolic"

				onClicked: root.composition.fileSelectionModel.selectFile()
			}

			Item {
				Layout.fillWidth: true
			}

			Controls.ToolButton {
				text: qsTr("Send")
				icon.name: "mail-send-symbolic"
				onClicked: {
					// always (re)set the body in root.composition (it may contain a body from a previous message)
					root.composition.body = messageText.text
					root.composition.send()
					close()
				}
			}
		}
	}
}
