// SPDX-FileCopyrightText: 2026 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan

/**
 * Form entries for blocking a JID and reporting spam/abuse.
 *
 * The entries for reporting are only shown if the server supports blocking reports.
 *
 * It is intended to be used within the contentItem of a FormCard.FormCard.
 */
ColumnLayout {
	id: root

	required property Account account
	property string jid
	// List of stanza ID references (QXmppStanzaId) of the reported messages.
	property var messageReferences: []
	readonly property bool reportsSupported: account.settings.blockingReportsSupported
	property bool reportingMandatory: false
	readonly property bool reportingEnabled: reportsSupported && (reportingMandatory || reportSwitch.checked)

	spacing: 0

	FormCard.FormSwitchDelegate {
		id: reportSwitch
		text: qsTr("Report")
		description: qsTr("Report spam or abuse")
		visible: root.reportsSupported && !root.reportingMandatory
		checked: root.reportingMandatory
		onToggled: {
			if (checked) {
				root.forceActiveFocus()
			}
		}
	}

	FormComboBoxDelegate {
		id: reasonDelegate
		text: qsTr("Reason")
		currentIndex: 0
		description: currentIndex < 0 ? "" : model[currentIndex].description
		visible: root.reportingEnabled
		textRole: "label"
		valueRole: "value"
		model: [
			{
				label: qsTr("Spam"),
				description: qsTr("Unsolicited messages, often sent in bulk"),
				value: SpamReport.Reason.Spam
			},
			{
				label: qsTr("Abuse"),
				description: qsTr("Harassing, threatening, or otherwise abusive behavior"),
				value: SpamReport.Reason.Abuse
			}
		]
	}

	FormCard.FormTextAreaDelegate {
		id: descriptionField
		visible: root.reportingEnabled
		label: qsTr("Description (optional)")
	}

	function forceActiveFocus() {
		if (!Kirigami.Settings.isMobile) {
			descriptionField.forceActiveFocus()
		}
	}

	function submit() {
		if (reportingEnabled) {
			account.blockingController.blockAndReport(jid, reasonDelegate.currentValue, descriptionField.text, messageReferences)
		} else {
			account.blockingController.block(jid)
		}
	}
}
