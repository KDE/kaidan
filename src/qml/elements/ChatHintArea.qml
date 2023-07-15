// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

/**
 * This is an area used for displaying hints and corresponding buttons on the chat page.
 *
 * It is opened by increasing its height and closed by decreasing it again.
 */
Rectangle {
	id: root

	property ChatHintModel chatHintModel
	property int index
	property string text
	property var buttons
	property bool loading
	property string loadingDescription

	width: ListView.view.width
	height: enabled ? contentArea.height : 0
	enabled: false
	clip: true
	color: primaryBackgroundColor
	ListView.delayRemove: true
	onLoadingChanged: loading ? loadingStackArea.showLoadingView() : loadingStackArea.hideLoadingView()
	onHeightChanged: {
		// Ensure the deletion of this item after the removal animation of decreasing its height.
		// When this item is removed from the model (i.e., it has the index -1), it is set to be
		// finally removed from the user interface as soon as it is completely collapsed.
		if (index === -1 && height === 0) {
			ListView.delayRemove = false
		}
	}

	Behavior on height {
		SmoothedAnimation {
			velocity: 550
		}
	}

	ColumnLayout {
		id: contentArea
		anchors.left: root.left
		anchors.right: root.right

		// top: colored separator
		Rectangle {
			height: 3
			color: Kirigami.Theme.highlightColor
			Layout.fillWidth: true
		}

		// middle: chat hint
		ColumnLayout {
			Layout.margins: Kirigami.Units.largeSpacing

			LoadingStackArea {
				id: loadingStackArea
				loadingArea.background.visible: false
				loadingArea.description: root.loadingDescription

				ColumnLayout {
					visible: root.text

					CenteredAdaptiveText {
						id: hintText
						text: root.text
					}
				}

				RowLayout {
					visible: root.buttons.length
					Layout.alignment: Qt.AlignHCenter
					Layout.maximumWidth: largeButtonWidth * root.buttons.length + spacing

					Repeater {
						id: buttonArea
						model: root.buttons
						delegate: CenteredAdaptiveButton {
							text: modelData.text
							onClicked: root.chatHintModel.handleButtonClicked(root.index, modelData.type)
						}
					}
				}
			}
		}

		// bottom: colored separator
		Rectangle {
			height: 3
			color: Kirigami.Theme.highlightColor
			Layout.fillWidth: true
		}
	}
}
