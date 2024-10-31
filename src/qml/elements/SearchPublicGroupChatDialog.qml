// SPDX-FileCopyrightText: 2022 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.19 as Kirigami
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

import im.kaidan.kaidan 1.0
import PublicGroupChats 1.0 as PublicGroupChats

Dialog {
	id: root

	property ListViewSearchField searchField
	property string errorMessage

	title: qsTr("Search public groups (%1)")
		.arg("%1/%2".arg(groupChatsProxy.count).arg(groupChatsModel.count))
	// "implicitHeight" is needed in addition to "preferredHeight" to avoid a binding loop.
	implicitHeight: maximumHeight
	preferredHeight: maximumHeight
	onOpened: {
		if (!Kirigami.Settings.isMobile) {
			searchField.forceActiveFocus()
		}

		groupChatsManager.requestAll()
	}

	ListView {
		id: groupChatListView
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
			Component.onCompleted: sort(0, Qt.DescendingOrder)
		}
		header:	MobileForm.FormCard {
			z: 3
			width: ListView.view.width
			Kirigami.Theme.colorSet: Kirigami.Theme.Window
			contentItem: MobileForm.AbstractFormDelegate {
				background: null
				contentItem: ColumnLayout {
					ListViewSearchField {
						listView: groupChatListView
						Layout.fillWidth: true
						onTextChanged: groupChatsProxy.setFilterWildcard(text)
						onActiveFocusChanged: {
							// Force the active focus when it is lost.
							if (!Kirigami.Settings.isMobile && !activeFocus) {
								forceActiveFocus()
							}
						}
						Component.onCompleted: root.searchField = this
					}

					LoadingStackArea {
						visible: busy || root.errorMessage
						loadingArea.background.visible: false
						loadingArea.description: qsTr("Downloading…")
						busy: groupChatsManager.isRunning

						RowLayout {
							spacing: 0

							Item {
								Layout.fillWidth: true
							}

							Controls.Label {
								text: root.errorMessage
								font.bold: true
								wrapMode: Text.Wrap
								padding: Kirigami.Units.largeSpacing * 2
								rightPadding: replyCancelingButton.width + replyCancelingButton.anchors.rightMargin * 2
								horizontalAlignment: Text.AlignHCenter
								Layout.topMargin: Kirigami.Units.largeSpacing
								Layout.maximumWidth: groupChatListView.width - Kirigami.Units.largeSpacing * 5
								background: RoundedRectangle {
									color: Kirigami.Theme.negativeBackgroundColor
								}

								ClickableIcon {
									id: replyCancelingButton
									Controls.ToolTip.text: qsTr("Retry")
									source: "view-refresh-symbolic"
									implicitHeight: Kirigami.Units.iconSizes.smallMedium
									anchors.right: parent.right
									anchors.rightMargin: Kirigami.Units.largeSpacing
									anchors.verticalCenter: parent.verticalCenter
									onClicked: {
										root.errorMessage = ""
										groupChatsManager.requestAll()
									}
								}
							}

							Item {
								Layout.fillWidth: true
							}
						}
					}
				}
			}
		}
		headerPositioning: ListView.OverlayHeader
		delegate: MobileForm.AbstractFormDelegate {
			width: ListView.view.width
			contentItem: RowLayout {
				spacing: 12

				ColumnLayout {
					Avatar {
						iconSource: "group"
						jid: model.address
						name: model.name
					}

					RowLayout {
						Kirigami.Icon {
							source: "group"
							Layout.preferredWidth: Kirigami.Units.iconSizes.small
							Layout.preferredHeight: Layout.preferredWidth
						}

						Controls.Label {
							text: model.users.toString()
							font.bold: true
						}
					}
				}

				ColumnLayout {
					RowLayout {
						Controls.Label {
							text: model.name
							wrapMode: Text.Wrap
							font.bold: true
							Layout.fillWidth: true
						}

						Controls.Label {
							text: model.languages.join(" ")
							color: Kirigami.Theme.disabledTextColor
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
						color: Kirigami.Theme.disabledTextColor
						Layout.fillWidth: true
					}
				}
			}
			onClicked: Qt.openUrlExternally(Utils.groupChatUri(model.groupChat))
		}
	}

	PublicGroupChats.SearchManager {
		id: groupChatsManager
		onError: root.errorMessage = qsTr("The public groups could not be retrieved: %1").arg(error)
	}
}
