/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2022 Kaidan developers and contributors
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
import org.kde.kirigami 2.12 as Kirigami
import "../elements"
import im.kaidan.kaidan 1.0
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

SettingsPageBase {
	title: qsTr("About Kaidan")

	implicitHeight: layout.implicitHeight
	implicitWidth: layout.implicitWidth

	ColumnLayout {
		id: layout
		anchors.fill: parent

		Layout.preferredWidth: 600

		MobileForm.FormCard {
			Layout.fillWidth: true

			contentItem: ColumnLayout {
				spacing: 0
				MobileForm.AbstractFormDelegate {
					Layout.fillWidth: true
					contentItem: RowLayout {
						Image {
							source: Utils.getResourcePath("images/kaidan.svg")
							Layout.preferredWidth: Kirigami.Units.gridUnit * 5
							Layout.preferredHeight: Kirigami.Units.gridUnit * 5
							Layout.fillWidth: true
							Layout.fillHeight: true
							Layout.alignment: Qt.AlignCenter
							fillMode: Image.PreserveAspectFit
							mipmap: true
							sourceSize: Qt.size(width, height)
						}
						ColumnLayout {
							Kirigami.Heading {
								text: Utils.applicationDisplayName() + " " + Utils.versionString()
								textFormat: Text.PlainText
								wrapMode: Text.WordWrap
								Layout.fillWidth: true
								horizontalAlignment: Qt.AlignLeft
							}

							Controls.Label {
								text: "<i>" + qsTr("A simple, user-friendly Jabber/XMPP client") + "</i>"
								textFormat: Text.StyledText
								wrapMode: Text.WordWrap
								Layout.fillWidth: true
								horizontalAlignment: Qt.AlignLeft
							}
						}
					}
				}
			}
		}
		MobileForm.FormCard {
			Layout.fillWidth: true

			contentItem: ColumnLayout {
				MobileForm.AbstractFormDelegate {
					Layout.fillWidth: true
					contentItem: ColumnLayout {
						Controls.Label {
							text: "License"
						}
						Controls.Label {
							font: Kirigami.Theme.smallFont
							text: "GPLv3+ / CC BY-SA 4.0"
							color: Kirigami.Theme.disabledTextColor
							textFormat: Text.PlainText
						}
					}
				}
				MobileForm.AbstractFormDelegate {
					Layout.fillWidth: true
					contentItem: ColumnLayout {
						Controls.Label {
							text: "Copyright"
						}
						Controls.Label {
							font: Kirigami.Theme.smallFont
							text: "© 2016-2023 Kaidan developers and contributors"
							color: Kirigami.Theme.disabledTextColor
							textFormat: Text.PlainText
						}
					}
				}
				MobileForm.FormButtonDelegate {
					text: qsTr("Report problems")
					description: qsTr("Report issues in Kaidan to the developers")
					onClicked: Qt.openUrlExternally(Utils.issueTrackingUrl)
					icon.name: "tools-report-bug"
				}
				MobileForm.FormButtonDelegate {
					text: qsTr("View source code online")
					description: qsTr("View Kaidan's source code and contribute to the project")
					onClicked: Qt.openUrlExternally(Utils.applicationSourceCodeUrl())
					icon.name: "kde"
				}
				MobileForm.FormButtonDelegate {
					text: qsTr("Visit Website")
					description: "kaidan.im"
					onClicked: Qt.openUrlExternally("https://www.kaidan.im")
					icon.name: "globe"
				}
			}
		}

		Item {
			Layout.fillHeight: true
		}
	}
}
