// SPDX-FileCopyrightText: 2023 Mathis Brüchert <mbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan

FormInfoContent {
	FormCard.FormCard {
		Layout.fillWidth: true

		UrlFormButtonDelegate {
			text: qsTr("Visit website")
			description: qsTr("Open Kaidan's website in a web browser")
			icon.name: "globe"
			url: Utils.applicationWebsiteUrl
		}

		UrlFormButtonDelegate {
			text: qsTr("Follow on Mastodon")
			description: qsTr("Open Kaidan's Mastodon page in a web browser")
			icon.name: "send-to-symbolic"
			url: Utils.mastodonUrl
		}

		UrlFormButtonDelegate {
			text: qsTr("Donate")
			description: qsTr("Support Kaidan's development and infrastructure by a donation")
			icon.name: "emblem-favorite-symbolic"
			url: Utils.donationUrl
		}

		UrlFormButtonDelegate {
			text: qsTr("Report problems")
			description: qsTr("Report issues with Kaidan to the developers")
			icon.name: "computer-fail-symbolic"
			url: Utils.issueTrackingUrl
		}

		UrlFormButtonDelegate {
			text: qsTr("View source code")
			description: qsTr("View Kaidan's source code online and contribute to the project")
			icon.name: "system-search-symbolic"
			url: Utils.applicationSourceCodeUrl
		}
	}

	FormCard.FormCard {
		Layout.fillWidth: true

		FormCard.AbstractFormDelegate {
			Layout.fillWidth: true
			background: null
			contentItem: ColumnLayout {
				Controls.Label {
					text: "GPLv3+ / CC BY-SA 4.0"
					textFormat: Text.PlainText
					wrapMode: Text.WordWrap
					Layout.fillWidth: true
				}

				Controls.Label {
					text: "License"
					font: Kirigami.Theme.smallFont
					color: Kirigami.Theme.disabledTextColor
					wrapMode: Text.WordWrap
					Layout.fillWidth: true
				}
			}
		}

		FormCard.AbstractFormDelegate {
			Layout.fillWidth: true
			background: null
			contentItem: ColumnLayout {
				Controls.Label {
					text: "© 2016-2025 Kaidan developers and contributors"
					textFormat: Text.PlainText
					wrapMode: Text.WordWrap
					Layout.fillWidth: true
				}

				Controls.Label {
					text: "Copyright"
					font: Kirigami.Theme.smallFont
					color: Kirigami.Theme.disabledTextColor
					wrapMode: Text.WordWrap
					Layout.fillWidth: true
				}
			}
		}

		FormCard.AbstractFormDelegate {
			Layout.fillWidth: true
			background: null
			contentItem: ColumnLayout {
				Controls.Label {
					text: "Kaidan is a project of the international free software community KDE"
					textFormat: Text.PlainText
					wrapMode: Text.WordWrap
					Layout.fillWidth: true
				}

				Controls.Label {
					text: "Community"
					font: Kirigami.Theme.smallFont
					color: Kirigami.Theme.disabledTextColor
					wrapMode: Text.WordWrap
					Layout.fillWidth: true
				}
			}
		}
	}
}
