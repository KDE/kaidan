// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.12 as Kirigami

import im.kaidan.kaidan 1.0
import PublicGroupChats 1.0 as PublicGroupChats

Kirigami.OverlaySheet {
	id: root

	signal openGroupChatRequested(var groupChat)

	parent: applicationWindow().overlay
	header: Kirigami.Heading {
		text: qsTr("Search public group chat (%1)")
				.arg(groupChatsManager.isRunning
					? qsTr("Loading...")
					: "%1/%2".arg(groupChatsProxy.count).arg(groupChatsModel.count))

		Layout.fillWidth: true
	}

	ColumnLayout {
		enabled: !groupChatsManager.isRunning

		RowLayout {
			Controls.TextField {
				id: filterField

				selectByMouse: true
				placeholderText: qsTr("Filter by %1...").arg(filterByMenuGroup.checkedAction.text.toLowerCase())

				onTextChanged: {
					groupChatsProxy.setFilterWildcard(text);
				}

				Layout.fillWidth: true
			}

			Controls.ToolButton {
				id: filterButton

				text: "⋮"

				onClicked: {
					filterMenu.open();
				}

				Controls.Menu {
					id: filterMenu

					// Ensure Popup above the sheet
					z: root.rootItem.z + 1

					Controls.Menu {
						id: filterByMenu

						title: qsTr("Filter by")
						// Ensure Popup above the sheet
						z: root.rootItem.z + 1

						Controls.ActionGroup {
							id: filterByMenuGroup

							readonly property string role: checkedAction ? checkedAction.role : PublicGroupChats.Model.CustomRole.Name
						}

						Controls.Action {
							readonly property int role: PublicGroupChats.Model.CustomRole.GlobalSearch

							text: qsTr("All")
							checkable: true
							checked: true
							shortcut: "Ctrl+A"

							Controls.ActionGroup.group: filterByMenuGroup
						}

						Controls.Action {
							readonly property int role: PublicGroupChats.Model.CustomRole.Name

							text: qsTr("Name")
							checkable: true
							shortcut: "Ctrl+N"

							Controls.ActionGroup.group: filterByMenuGroup
						}

						Controls.Action {
							readonly property int role: PublicGroupChats.Model.CustomRole.Description

							text: qsTr("Description")
							checkable: true
							shortcut: "Ctrl+D"

							Controls.ActionGroup.group: filterByMenuGroup
						}

						Controls.Action {
							readonly property int role: PublicGroupChats.Model.CustomRole.Address

							text: qsTr("Address")
							checkable: true
							shortcut: "Ctrl+D"

							Controls.ActionGroup.group: filterByMenuGroup
						}
					}

					Controls.Menu {
						id: filterByLanguage

						title: qsTr("Language")
						// Ensure Popup above the sheet
						z: root.rootItem.z + 1

						Controls.ButtonGroup {
							id: filterByLanguageGroup

							readonly property string language: checkedButton ? checkedButton.text : ""
						}

						Repeater {
							model: groupChatsModel.languages
							delegate: Controls.MenuItem {
								text: model.modelData
								checkable: true
								checked: text === ""

								Controls.ButtonGroup.group: filterByLanguageGroup
							}
						}
					}
				}
			}

			Layout.fillWidth: true
		}

		ListView {
			id: groupChatsField

			clip: true
			model: PublicGroupChats.ProxyModel {
				id: groupChatsProxy

				languageFilter: filterByLanguageGroup.language
				filterCaseSensitivity: Qt.CaseInsensitive
				filterRole: filterByMenuGroup.role
				sortCaseSensitivity: Qt.CaseInsensitive
				sortRole: PublicGroupChats.Model.CustomRole.Name
				sourceModel: PublicGroupChats.Model {
					id: groupChatsModel

					groupChats: groupChatsManager.cachedGroupChats
				}
			}

			delegate: Controls.SwipeDelegate {
				width: ListView.view.width
				contentItem: RowLayout {
					spacing: 12

					Rectangle {
						width: 48
						height: width
						color: "lightGray"

						border {
							width: 2
							color: "black"
						}

						Text {
							text: model.name.charAt(0).toUpperCase()
							horizontalAlignment: Text.AlignHCenter
							verticalAlignment: Text.AlignVCenter

							font {
								bold: true
								pixelSize: 30
							}

							anchors {
								fill: parent
							}
						}

						Layout.alignment: Qt.AlignTop
					}

					ColumnLayout {
						RowLayout {
							Text {
								text: model.name
								wrapMode: Text.Wrap

								font {
									bold: true
								}
							}

							Text {
								text: model.languages.join(" ")
								color: "gray"
							}

							Layout.fillWidth: true
						}

						Text {
							text: model.description
							wrapMode: Text.Wrap

							Layout.fillWidth: true
						}

						Text {
							text: model.address
							wrapMode: Text.Wrap
							color: "gray"

							Layout.fillWidth: true
						}
					}
				}

				onClicked: {
					root.openGroupChatRequested(model.groupChat);
				}
			}

			PublicGroupChats.SearchManager {
				id: groupChatsManager
			}

			Layout.fillWidth: true
			Layout.minimumHeight: 300
		}

		Layout.fillWidth: true
	}

	Component.onCompleted: {
		groupChatsManager.requestAll();
	}

	onSheetOpenChanged: {
		if (!sheetOpen) {
			filterMenu.close();
		}
	}
}
