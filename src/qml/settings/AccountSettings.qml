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
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import QtQml 2.14
import org.kde.kirigami 2.12 as Kirigami
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

import im.kaidan.kaidan 1.0
import "../elements"

SettingsPageBase {
	id: root
	title: qsTr("Account settings")

	implicitHeight: content.implicitHeight
	implicitWidth: content.implicitWidth

	readonly property var chatSupportList: providerListModel.providerFromBareJid(root.jid).chatSupport
	readonly property var groupChatSupportList: providerListModel.providerFromBareJid(root.jid).groupChatSupport

	readonly property string jid: AccountManager.jid
	readonly property string displayName: AccountManager.displayName

	UserResourcesWatcher {
		id: ownResourcesWatcher
		jid: root.jid
	}

	UserPresenceWatcher {
		id: userPresence
		jid: root.jid
	}

	ProviderListModel {
		id: providerListModel
	}

	ColumnLayout {
		id: content
		anchors.fill: parent
		Layout.preferredWidth: 600
		Item {
			Layout.preferredHeight: 10
		}
		MobileForm.FormCard {
			Layout.fillWidth: true

			contentItem: ColumnLayout {
				spacing: 0

				MobileForm.AbstractFormDelegate {
					background: Item{}
					Layout.fillWidth: true
					contentItem: RowLayout {
						Layout.alignment: Qt.AlignTop
						Layout.maximumWidth: largeButtonWidth
						spacing: 20

						Avatar {
							MouseArea {
								anchors.fill: parent
								hoverEnabled: true
								cursorShape: Qt.PointingHandCursor

								onClicked: {
									stack.push("ChangeAvatar.qml")
								}

								onEntered: {
									avatarChangeHoverImage.visible = true
									avatarHoverFadeInAnimation.start()
								}

								onExited: {
									avatarHoverFadeOutAnimation.start()
								}
								Rectangle{
									id: avatarChangeHoverImage

									color: Kirigami.ColorUtils.linearInterpolation("black", "transparent", 0.5)
									anchors.fill: parent
									radius: parent.height/2
									visible: false

									NumberAnimation on opacity {
										id: avatarHoverFadeInAnimation
										from: 0
										to: 1
										duration: 250
									}

									NumberAnimation on opacity {
										id: avatarHoverFadeOutAnimation
										from: 1
										to: 0
										duration: 250
									}
									Kirigami.Icon {
										source: "camera-photo-symbolic"
										color: Kirigami.Theme.backgroundColor
										width: parent.width / 2
										height: width

										anchors.centerIn: parent
									}
								}
							}

							Rectangle {
								Rectangle {
									color: userPresence.availabilityColor
									radius: 50
									height: 25
									width: 25
									x: (parent.width - width) / 2
									y: (parent.height - height) / 2
								}
								color: Kirigami.Theme.backgroundColor
								radius: 50
								height: 35
								width: 35
								x: Kirigami.Units.gridUnit * 5
								y: Kirigami.Units.gridUnit * 5
								visible: userPresence.availability !== Presence.Offline
							}
							Layout.margins: 10
							implicitHeight: Kirigami.Units.gridUnit * 7
							implicitWidth: Kirigami.Units.gridUnit * 7
							jid: root.jid
							name: root.displayName
						}


						ColumnLayout {
							RowLayout {
								id: displayNameHeader
								Layout.fillWidth: true
								Kirigami.Heading {

									Layout.fillWidth: true
									text: root.displayName
									textFormat: Text.PlainText
									maximumLineCount: 2
									elide: Text.ElideRight

									// TODO how to get update of current vCard using Entity Capabilities?
									onTextChanged: Kaidan.client.vCardManager.vCardRequested(root.jid)
								}
								Controls.ToolButton{
									icon.name: "edit-symbolic"
									onClicked: {
										displayNameEditLayout.visible = true
										displayNameTextField.forceActiveFocus()
									}
								}
							}

							RowLayout {
								id: displayNameEditLayout
								visible: false
								onVisibleChanged: displayNameHeader.visible = !visible

								Layout.fillWidth: true
								Controls.TextField {
									id: displayNameTextField
									text: displayNameHeader.text
									Layout.fillWidth: true

									z: 1

									onAccepted: {
										if (text !== root.displayName) {
											Kaidan.client.vCardManager.changeNicknameRequested(text)
										}
										parent.visible = false
									}
								}
								Controls.ToolButton{
									icon.name: "answer-correct"
									onClicked: {
										if (displayNameTextField.text !== root.displayName) {
											Kaidan.client.vCardManager.changeNicknameRequested(displayNameTextField.text)
										}
										parent.visible = false
									}
								}
							}

							Controls.Label {
								text: root.jid
								color: Kirigami.Theme.disabledTextColor
								textFormat: Text.PlainText
							}
						}
					}
				}
			}
		}

		MobileForm.FormCard {
			visible: infoRepeater.count !== 0
			Layout.fillWidth: true

			contentItem: ColumnLayout {
				spacing: 0
				MobileForm.FormCardHeader {
					title: qsTr("Profile")
				}
				Repeater {
					id: infoRepeater
					model: VCardModel {
						jid: root.jid
					}

					delegate: MobileForm.AbstractFormDelegate {
						Layout.fillWidth: true
						contentItem: ColumnLayout {
							Controls.Label {
								text: Utils.formatMessage(model.value)
								onLinkActivated: Qt.openUrlExternally(link)
								textFormat: Text.StyledText
							}
							Controls.Label {
								text: model.key
								color: Kirigami.Theme.disabledTextColor
								textFormat: Text.PlainText
								font: Kirigami.Theme.smallFont
							}
						}
					}
				}
			}
		}

		MobileForm.FormCard {
			Layout.fillWidth: true

			contentItem: ColumnLayout {
				spacing: 0
				MobileForm.FormCardHeader {
					title: qsTr("Online devices")
				}

				Repeater {
					id: deviceRepeater
					Layout.fillHeight: true

					model: UserDevicesModel {
						jid: root.jid
					}
					delegate: MobileForm.AbstractFormDelegate {
						Layout.fillWidth: true
						contentItem: ColumnLayout {
							Controls.Label {
								text: {
									if (model.name) {
										if (model.version) {
											return model.name + " v" + model.version
										}
										return model.name
									}
									return model.resource
								}
							}
							Controls.Label {
								color: Kirigami.Theme.disabledTextColor
								text: model.os
								font: Kirigami.Theme.smallFont
							}
						}
					}
				}

			}
		}

		MobileForm.FormCard {
			Layout.fillWidth: true

			contentItem: ColumnLayout {
				readonly property string providerUrl: {
					const domain = root.jid.split('@')[1]
					const provider = providerListModel.provider(domain)
					const website = providerListModel.chooseWebsite(provider.websites)

					return website ? website : "https://" + domain
				}

				spacing: 0
				MobileForm.FormCardHeader {
					title: qsTr("Provider")
				}

				MobileForm.FormButtonDelegate {
					text: qsTr("Visit website")
					description: qsTr("Open the website of your provider in a browser")
					onClicked: {
						Qt.openUrlExternally(parent.providerUrl)
					}
					icon.name: "internet-services"
				}
				MobileForm.FormButtonDelegate {
					text: qsTr("Copy URL")
					description: qsTr("Copy the providers web address to the clipboard")
					onClicked: {
						Utils.copyToClipboard(parent.providerUrl)
						passiveNotification(qsTr("URL copied to clipboard."))
					}
					icon.name: "edit-copy"
				}
				MobileForm.FormCardHeader {
					title: qsTr("Support")
					visible: chatSupportButton.visible || groupChatSupportButton.visible

				}
				RosterAddContactSheet {
					id: addContactSheet
				}

				ChatSupportSheet {
					id: chatSupportSheet
				}
				MobileForm.FormButtonDelegate {
					id: chatSupportButton
					text: qsTr("Support")
					icon.name: "help-contents"
					description: qsTr("Start chat with your provider's support contact")

					visible: chatSupportList.length > 0
					onClicked: {
						if (chatSupportList.length === 1) {
							if (!addContactSheet.sheetOpen) {
								addContactSheet.jid = chatSupportList[0]
								addContactSheet.nickname = chatSupportButton.text
								addContactSheet.open()
							}
						} else {
							chatSupportSheet.isGroupChatSupportSheet = false
							chatSupportSheet.chatSupportList = chatSupportList

							if (!chatSupportSheet.sheetOpen) {
								chatSupportSheet.open()
							}
						}
					}
				}
				MobileForm.FormButtonDelegate {
					id: groupChatSupportButton
					text: qsTr("Support Group")
					icon.name: "group"
					description: qsTr("Join your provider's support group")
					visible: groupChatSupportList.length > 0

					onClicked: {
						if (groupChatSupportList.length === 1) {
							Qt.openUrlExternally("xmpp:" + groupChatSupportList[0] + "?join")
						} else {
							chatSupportSheet.isGroupChatSupportSheet = true
							chatSupportSheet.chatSupportList = groupChatSupportList

							if (!chatSupportSheet.sheetOpen) {
								chatSupportSheet.open()
							}
						}
					}
				}
			}
		}

		Item {
			visible: Kirigami.Settings.isMobile
			Layout.preferredHeight: 60
		}
	}
}
