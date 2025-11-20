// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan

/**
 * Used to filter the displayed providers.
 */
FormCard.FormCard {
	id: root

	property ProviderFilterModel providerFilterModel

	FormCard.FormSwitchDelegate {
		text: qsTr("Registration via Kaidan")
		description: qsTr("Show only providers that allow to create an account directly via Kaidan")
		checked: root.providerFilterModel.supportsInBandRegistrationOnly
		onToggled: root.providerFilterModel.supportsInBandRegistrationOnly = checked
	}

	FormCard.FormSwitchDelegate {
		text: qsTr("Free of charge")
		description: qsTr("Show only providers that are free of charge")
		checked: root.providerFilterModel.freeOfChargeOnly
		onToggled: root.providerFilterModel.freeOfChargeOnly = checked
	}

	FormCard.FormSwitchDelegate {
		text: qsTr("Password reset")
		description: qsTr("Show only providers that allow to reset your password")
		checked: root.providerFilterModel.supportsPasswordResetOnly
		onToggled: root.providerFilterModel.supportsPasswordResetOnly = checked
	}

	FormCard.FormSwitchDelegate {
		text: qsTr("Green hosting")
		description: qsTr("Show only providers whose service runs on renewable energy")
		checked: root.providerFilterModel.hostedGreenOnly
		onToggled: root.providerFilterModel.hostedGreenOnly = checked
	}

	FormCard.FormComboBoxDelegate {
		text: qsTr("Bus factor")
		description: "Show only providers that have at least the selected bus factor"
		model: Array.from(Array(root.providerFilterModel.sourceModel.maximumBusFactor), (_, i) => i + 1)
		currentIndex: root.providerFilterModel.mininumBusFactor - 1
		onCurrentValueChanged: root.providerFilterModel.mininumBusFactor = currentValue
	}

	FormCard.FormComboBoxDelegate {
		text: qsTr("Location")
		description: qsTr("Show only providers that store and process their data in the selected country")
		model: root.providerFilterModel.sourceModel.availableFlags
		currentIndex: model.indexOf(root.providerFilterModel.flag)
		onCurrentValueChanged: root.providerFilterModel.flag = currentValue
	}

	FormCard.FormComboBoxDelegate {
		text: qsTr("Size of shared media")
		description: "Show only providers that allow to share media of at least the selected size in MB"
		model: root.providerFilterModel.httpUploadFileSizes
		currentIndex: model.indexOf(root.providerFilterModel.minimumHttpUploadFileSize)
		onCurrentValueChanged: root.providerFilterModel.minimumHttpUploadFileSize = currentValue
	}

	FormCard.FormComboBoxDelegate {
		text: qsTr("Size of all shared media in total")
		description: "Show only providers that store shared media up to at least the selected size in MB"
		model: root.providerFilterModel.httpUploadTotalSizes
		currentIndex: model.indexOf(root.providerFilterModel.minimumHttpUploadTotalSize)
		onCurrentValueChanged: root.providerFilterModel.minimumHttpUploadTotalSize = currentValue
	}

	FormCard.FormComboBoxDelegate {
		text: qsTr("Media storage duration")
		description: "Show only providers that store media for at least the selected duration in days"
		model: root.providerFilterModel.httpUploadStorageDurations
		currentIndex: model.indexOf(root.providerFilterModel.minimumHttpUploadStorageDuration)
		onCurrentValueChanged: root.providerFilterModel.minimumHttpUploadStorageDuration = currentValue
	}

	FormCard.FormComboBoxDelegate {
		text: qsTr("Message storage duration")
		description: "Show only providers that store messages for at least the selected duration in days"
		model: root.providerFilterModel.messageStorageDurations
		currentIndex: model.indexOf(root.providerFilterModel.minimumMessageStorageDuration)
		onCurrentValueChanged: root.providerFilterModel.minimumMessageStorageDuration = currentValue
	}

	FormCard.FormComboBoxDelegate {
		text: qsTr("Availability")
		description: "Show only providers that are available or listed for the selected count of years"
		model: root.providerFilterModel.availabilities
		currentIndex: model.indexOf(root.providerFilterModel.minimumAvailability)
		onCurrentValueChanged: root.providerFilterModel.minimumAvailability = currentValue
	}

	FormCard.FormComboBoxDelegate {
		text: qsTr("Organization")
		description: "Show only providers with the selected form of organization"
		model: root.providerFilterModel.sourceModel.availableOrganizations
		currentIndex: model.indexOf(root.providerFilterModel.organization)
		onCurrentValueChanged: root.providerFilterModel.organization = currentValue
	}
}
