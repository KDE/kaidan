// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami

/**
 * This page is the base of decision pages with two actions.
 *
 * Each action has an own image for describing its purpose.
 */
Kirigami.Page {
	property alias topDescription: topDescription.text
	property alias bottomDescription: bottomDescription.text

	property alias topImageSource: topImage.source
	property alias bottomImageSource: bottomImage.source

	property Kirigami.Action topAction
	property Kirigami.Action bottomAction

	property bool topActionAsMainAction: false

	property int descriptionMargin: 10

	ColumnLayout {
		anchors.fill: parent

		ColumnLayout {
			Layout.maximumWidth: largeButtonWidth
			Layout.alignment:  Qt.AlignCenter

			// image to show above the top action
			Kirigami.Icon {
				id: topImage
				Layout.fillWidth: true
				Layout.fillHeight: true
			}

			// description for the top action
			CenteredAdaptiveText {
				id: topDescription
				Layout.bottomMargin: descriptionMargin
			}

			// button for the top action
			CenteredAdaptiveButton {
				visible: !topActionAsMainAction
				text: topAction.text
				icon.name: topAction.icon.name
				onClicked: topAction.trigger()
				enabled: topAction.enabled
			}

			// button for the top action as main action
			CenteredAdaptiveHighlightedButton {
				visible: topActionAsMainAction
				text: topAction.text
				icon.name: topAction.icon.name
				onClicked: topAction.trigger()
				enabled: topAction.enabled
			}

			// horizontal line to separate the two actions
			Kirigami.Separator {
				Layout.fillWidth: true
				Layout.leftMargin: - (root.width - parent.width) / 4
				Layout.rightMargin: Layout.leftMargin
			}

			// button for the bottom action
			CenteredAdaptiveButton {
				text: bottomAction.text
				icon.name: bottomAction.icon.name
				onClicked: bottomAction.trigger()
				enabled: bottomAction.enabled
			}

			// description for the bottom action
			CenteredAdaptiveText {
				id: bottomDescription
				Layout.topMargin: descriptionMargin
			}

			// image to show below the bottom action
			Kirigami.Icon {
				id: bottomImage
				Layout.fillWidth: true
				Layout.fillHeight: true
			}
		}
	}
}
