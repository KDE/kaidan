// SPDX-FileCopyrightText: 2022 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan

Dialog {
	id: root

	property ListViewSearchField searchField
	property string errorMessage

	title: qsTr("Search public groups (%1)")
		.arg("%1/%2".arg(publicGroupChatProxyModel.count).arg(publicGroupChatModel.count))
	// "implicitHeight" is needed in addition to "preferredHeight" to avoid a binding loop.
	implicitHeight: maximumHeight
	preferredHeight: maximumHeight
	onOpened: {
		if (!Kirigami.Settings.isMobile) {
			searchField.forceActiveFocus()
		}

		publicGroupChatSearchController.requestAll()
	}

	ListView {
		id: groupChatListView
		clip: true
		model: PublicGroupChatProxyModel {
			id: publicGroupChatProxyModel
			filterCaseSensitivity: Qt.CaseInsensitive
			filterRole: PublicGroupChatModel.CustomRole.GlobalSearch
			sortCaseSensitivity: Qt.CaseInsensitive
			sortRole: PublicGroupChatModel.CustomRole.Users
			sourceModel: PublicGroupChatModel {
				id: publicGroupChatModel
				groupChats: publicGroupChatSearchController.cachedGroupChats
			}
			Component.onCompleted: sort(0, Qt.DescendingOrder)
		}
		header:	FormCard.FormCard {
			z: 3
			width: ListView.view.width
			Kirigami.Theme.colorSet: Kirigami.Theme.Window

			FormCard.AbstractFormDelegate {
				background: null
				contentItem: ColumnLayout {
					ListViewSearchField {
						listView: groupChatListView
						Layout.fillWidth: true
						onTextChanged: publicGroupChatProxyModel.setFilterWildcard(text)
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
						busy: publicGroupChatSearchController.isRunning

						RowLayout {
							spacing: 0

							Item {
								Layout.fillWidth: true
							}

							Controls.Label {
								text: root.errorMessage
								font.weight: Font.Medium
								wrapMode: Text.Wrap
								padding: Kirigami.Units.largeSpacing * 2
								rightPadding: retryButton.width + retryButton.anchors.rightMargin * 2
								horizontalAlignment: Text.AlignHCenter
								Layout.topMargin: Kirigami.Units.largeSpacing
								Layout.maximumWidth: groupChatListView.width - Kirigami.Units.largeSpacing * 5
								background: RoundedRectangle {
									color: Kirigami.Theme.negativeBackgroundColor
								}

								ClickableIcon {
									id: retryButton
									Controls.ToolTip.text: qsTr("Retry")
									source: "view-refresh-symbolic"
									implicitHeight: Kirigami.Units.iconSizes.smallMedium
									anchors.right: parent.right
									anchors.rightMargin: Kirigami.Units.largeSpacing
									anchors.verticalCenter: parent.verticalCenter
									onClicked: {
										root.errorMessage = ""
										publicGroupChatSearchController.requestAll()
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
		delegate: FormCard.AbstractFormDelegate {
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
							font.weight: Font.Medium
						}
					}
				}

				ColumnLayout {
					RowLayout {
						Controls.Label {
							text: model.name
							wrapMode: Text.Wrap
							font.weight: Font.Medium
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
			onClicked: Qt.openUrlExternally(Utils.groupChatUri(model.address))
		}
	}

	PublicGroupChatSearchController {
		id: publicGroupChatSearchController
		onError: root.errorMessage = qsTr("The public groups could not be retrieved: %1").arg(error)
	}

	Connections {
		target: MainController

		function onOpenChatPageRequested(accountJid, chatJid) {
			root.close()
		}
	}
}
