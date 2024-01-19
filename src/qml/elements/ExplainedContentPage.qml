// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Mathis Brüchert <mbblp@protonmail.ch>
// SPDX-FileCopyrightText: 2021 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

/**
 * This page is the base for pages with content needing an explanation.
 *
 * It consists of a centered content area and an overlay containing an explanation area with a background and two buttons.
 */
Kirigami.Page {
	leftPadding: 0
	rightPadding: 0
	topPadding: 0
	bottomPadding: 0

	/**
	 * area containing the explanation displayed while the content is not displayed
	 */
	property alias explanationArea: explanationArea

	/**
	 * explanation within the explanation area
	 */
	property alias explanation: explanationArea.data

	/**
	 * background of the explanation area
	 */
	property alias explanationAreaBackground: explanationAreaBackground

	/**
	 * content displayed while the explanation is not displayed
	 */
	property alias content: contentArea.data

	/**
	 * button for a primary action
	 */
	property alias primaryButton: primaryButton

	/**
	 * button for a secondary action
	 */
	property alias secondaryButton: secondaryButton

	/**
	 * true to have a margin between the content and the window's border, otherwise false
	 */
	property bool useMarginsForContent: true

	Item {
		id: contentArea
		anchors.fill: parent
		anchors.margins: useMarginsForContent ? 20 : 0
		anchors.bottomMargin: useMarginsForContent ? parent.height - buttonArea.y : 0
	}

	// background of overlay
	RoundedRectangle {
		id: explanationAreaBackground
		z: 1
		anchors.fill: overlay
		anchors.margins: -8
		color: Kirigami.Theme.backgroundColor
		opacity: 0.9
		visible: explanationArea.visible
	}

	ColumnLayout {
		id: overlay
		z: 2
		anchors.fill: parent
		anchors.margins: 18

		Item {
			id: explanationArea
			Layout.fillWidth: true
			Layout.fillHeight: true
			Layout.bottomMargin: Kirigami.Units.smallSpacing * 3
		}

		ColumnLayout {
			id: buttonArea
			Layout.alignment: Qt.AlignBottom | Qt.AlignHCenter
			Layout.maximumWidth: largeButtonWidth

			CenteredAdaptiveHighlightedButton {
				id: primaryButton
			}

			CenteredAdaptiveButton {
				id: secondaryButton
			}
		}
	}
}
