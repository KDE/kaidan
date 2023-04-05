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
		}
	}

	Item {
		Connections {
			target: root.composition.fileSelectionModel

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
					root.composition.send()
					close()
				}
			}
		}
	}
}
