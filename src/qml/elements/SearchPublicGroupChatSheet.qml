// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.14 as Kirigami

import im.kaidan.kaidan 1.0
import PublicGroupChats 1.0 as PublicGroupChats

Kirigami.OverlaySheet {
	id: root

	signal openGroupChatRequested(var groupChat)

	parent: applicationWindow().overlay
	header: Kirigami.Heading {
		text: qsTr("Search public group chat (%1)")
				.arg("%1/%2".arg(groupChatsProxy.count).arg(groupChatsModel.count))

		wrapMode: Text.WordWrap
	}

	ColumnLayout {
		enabled: !groupChatsManager.isRunning

		Controls.TextField {
			id: filterField

			selectByMouse: true
			placeholderText: qsTr("Search…")

			onTextChanged: {
				groupChatsProxy.setFilterWildcard(text);
			}

			Layout.fillWidth: true
		}

		ListView {
			id: groupChatsField

			clip: true
			model: PublicGroupChats.ProxyModel {
				id: groupChatsProxy

				filterCaseSensitivity: Qt.CaseInsensitive
				filterRole: PublicGroupChats.Model.CustomRole.GlobalSearch
				sortCaseSensitivity: Qt.CaseInsensitive
				sortRole: PublicGroupChats.Model.CustomRole.Users
				sourceModel: PublicGroupChats.Model {
					id: groupChatsModel

					groupChats: groupChatsManager.cachedGroupChats
				}

				Component.onCompleted: {
					sort(0, Qt.DescendingOrder);
				}
			}

			Controls.ScrollBar.vertical: Controls.ScrollBar {
			}

			delegate: Controls.SwipeDelegate {
				width: ListView.view.width - ListView.view.Controls.ScrollBar.vertical.width

				contentItem: RowLayout {
					spacing: 12

					ColumnLayout {
						Avatar {
							width: 48
							height: width
							jid: model.address
							name: model.name
							iconSource: "group"
						}

						RowLayout {
							Kirigami.Icon {
								source: "group"

								Layout.preferredWidth: Kirigami.Units.iconSizes.small
								Layout.preferredHeight: Layout.preferredWidth
							}

							Controls.Label {
								text: model.users.toString()

								font {
									bold: true
								}
							}
						}
					}

					ColumnLayout {
						RowLayout {
							Controls.Label {
								text: model.name
								wrapMode: Text.Wrap

								font {
									bold: true
								}

								Layout.fillWidth: true
							}

							Controls.Label {
								text: model.languages.join(" ")
								color: "gray"

								Layout.alignment: Qt.AlignTop
							}
						}

						Controls.Label {
							text: model.description
							wrapMode: Text.Wrap

							Layout.fillWidth: true
						}

						Controls.Label {
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

			Item {
				anchors.centerIn: parent

				// background of loadingArea
				Rectangle {
					anchors.fill: loadingArea
					anchors.margins: -8
					radius: roundedCornersRadius
					color: Kirigami.Theme.backgroundColor
					opacity: 0.9
					visible: loadingArea.visible
				}

				ColumnLayout {
					id: loadingArea
					anchors.centerIn: parent
					visible: groupChatsManager.isRunning

					Controls.BusyIndicator {
						Layout.alignment: Qt.AlignHCenter
					}

					Controls.Label {
						text: "<i>" + qsTr("Loading…") + "</i>"
						color: Kirigami.Theme.textColor
					}
				}
			}

			Layout.fillWidth: true
			Layout.minimumHeight: 300
		}
	}

	Component.onCompleted: {
		groupChatsManager.requestAll();
	}
}
