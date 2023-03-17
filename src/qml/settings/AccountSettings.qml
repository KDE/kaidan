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

		anchors.centerIn: parent

		Item {
			Layout.preferredHeight: 10
		}

		RowLayout {
			Layout.alignment: Qt.AlignTop
			Layout.maximumWidth: largeButtonWidth
			spacing: 20

			Avatar {
				Layout.preferredHeight: Kirigami.Units.gridUnit * 10
				Layout.preferredWidth: Kirigami.Units.gridUnit * 10
				jid: root.jid
				name: root.displayName

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

					Kirigami.Icon {
						id: avatarChangeHoverImage
						source: "camera-photo-symbolic"
						color: Kirigami.Theme.backgroundColor
						width: parent.width / 2
						height: width

						anchors.centerIn: parent

						opacity: 0
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
					}
				}
			}

			ColumnLayout {
				Kirigami.Heading {
					id: displayNameHeader

					Layout.fillWidth: true
					text: root.displayName
					textFormat: Text.PlainText
					maximumLineCount: 2
					elide: Text.ElideRight

					MouseArea {
						anchors.fill: parent

						cursorShape: Qt.PointingHandCursor

						onClicked: {
							displayNameTextField.visible = true
							displayNameTextField.forceActiveFocus()
						}

						Kirigami.Icon {
							source: "edit-symbolic"
							width: 15
							height: width

							anchors.right: parent.right
							anchors.leftMargin: 10
						}
					}

					// TODO how to get update of current vCard using Entity Capabilities?
					onTextChanged: Kaidan.client.vCardManager.vCardRequested(root.jid)
				}

				Controls.TextField {
					id: displayNameTextField
					text: displayNameHeader.text

					Layout.bottomMargin: Kirigami.Settings.isMobile ? -10 : -6

					visible: false
					z: 1

					onAccepted: {
						if (text !== root.displayName) {
							Kaidan.client.vCardManager.changeNicknameRequested(text)
						}

						visible = false
					}

					onVisibleChanged: displayNameHeader.visible = !visible
				}

				Controls.Label {
					text: root.jid
					color: Kirigami.Theme.disabledTextColor
					textFormat: Text.PlainText
				}

				RowLayout {
					spacing: Kirigami.Units.smallSpacing

					Kirigami.Icon {
						source: userPresence.availabilityIcon
						width: 26
						height: 26
					}

					Controls.Label {
						Layout.alignment: Qt.AlignVCenter
						text: userPresence.availabilityText
						color: userPresence.availabilityColor
						textFormat: Text.PlainText
					}

					Item {
						Layout.fillWidth: true
					}
				}
			}
		}

		Item {
			height: Kirigami.Units.largeSpacing
		}

		Kirigami.Heading {
			level: 2
			text: qsTr("Profile")
		}

		Repeater {
			model: VCardModel {
				jid: root.jid
			}

			delegate: ColumnLayout {
				Layout.fillWidth: true

				Controls.Label {
					text: Utils.formatMessage(model.value)
					onLinkActivated: Qt.openUrlExternally(link)
					textFormat: Text.StyledText
				}

				Controls.Label {
					text: model.key
					color: Kirigami.Theme.disabledTextColor
					textFormat: Text.PlainText
				}

				Item {
					height: 3
				}
			}
		}

		Item {
			height: Kirigami.Units.largeSpacing
		}

		Kirigami.Heading {
			level: 2
			text: qsTr("Online devices")
		}

		UserDeviceList {
			userJid: root.jid
		}

		Item {
			height: Kirigami.Units.largeSpacing
		}

		Kirigami.Heading {
			level: 2
			text: qsTr("Provider")
		}

		ColumnLayout {
			Layout.maximumWidth: largeButtonWidth

			readonly property string providerUrl: {
				var domain = root.jid.split('@')[1]
				var provider = providerListModel.provider(domain)
				var website = providerListModel.chooseWebsite(provider.websites)

				return website ? website : "https://" + domain
			}

			// TODO maybe add uploadLimits etc.

			CenteredAdaptiveHighlightedButton {
				text: qsTr("Visit website")
				icon.name: "web-browser-symbolic"

				onClicked: {
					Qt.openUrlExternally(parent.providerUrl)
				}
			}

			CenteredAdaptiveButton {
				text: qsTr("Copy URL")
				icon.name: "edit-copy-symbolic"

				onClicked: {
					Utils.copyToClipboard(parent.providerUrl)
					passiveNotification(qsTr("URL successfully copied to clipboard."))
				}
			}

			Item {
				height: Kirigami.Units.largeSpacing
			}

			Kirigami.Heading {
				level: 2
				text: qsTr("Support")
				visible: chatSupportButton.visible || groupChatSupportButton.visible
			}

			RosterAddContactSheet {
				id: addContactSheet
			}

			ChatSupportSheet {
				id: chatSupportSheet
			}

			CenteredAdaptiveHighlightedButton {
				id: chatSupportButton
				text: qsTr("Support")
				icon.name: "chat-symbolic"
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

			CenteredAdaptiveButton {
				id: groupChatSupportButton
				text: qsTr("Support Group")
				icon.name: "chat-symbolic"
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

		// placeholder for left, right and main action
		Item {
			visible: Kirigami.Settings.isMobile
			Layout.preferredHeight: 60
		}
	}
}
