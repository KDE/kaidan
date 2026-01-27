// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

/**
 * This is a base for pages containing a list view.
 *
 * It adds a search bar for searching items within the list view.
 */
Kirigami.ScrollablePage {
	id: root

	property alias searchField: searchField
	property alias toolbarItems: toolbarContent.data
	property bool separatorVisible: !searchField.listView?.atYBeginning

	background: Rectangle {
		color: primaryBackgroundColor
	}
	bottomPadding: 0
	header: Controls.Control {
		leftPadding: Kirigami.Units.smallSpacing * 3
		rightPadding: Kirigami.Units.smallSpacing * 3
		implicitHeight: pageStack.globalToolBar.preferredHeight
		background: ColumnLayout {
			spacing: 0

			Rectangle {
				color: primaryBackgroundColor
				Layout.fillWidth: true
				Layout.fillHeight: true
			}

			HorizontalSeparator {
				opacity: root.separatorVisible ? 0.5 : 0
				visible: opacity
				implicitHeight: 2

				Behavior on opacity {
					NumberAnimation {}
				}
			}
		}
		contentItem: RowLayout {
			id: toolbarContent
			spacing: Kirigami.Units.mediumSpacing * 2

			Loader {
				sourceComponent: pageStack.items[0] === root ? drawerHandle : (pageStack.wideMode ? null : backButton)
				visible: item
				Layout.leftMargin: - 3
				Layout.rightMargin: - 3

				Component {
					id: drawerHandle

					ToolbarButton {
						Controls.ToolTip.text: qsTr("Open menu")
						source: "open-menu-symbolic"
						onClicked: globalDrawer.open()
					}
				}

				Component {
					id: backButton

					ToolbarButton {
						Controls.ToolTip.text: qsTr("Go back")
						source: "go-previous-symbolic"
						onClicked: pageStack.goBack()
					}
				}
			}

			ListViewSearchField {
				id: searchField

				property bool busy: false

				font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.1
				Layout.fillWidth: true

				Rectangle {
					color: secondaryBackgroundColor
					visible: searchField.busy
					implicitWidth: Kirigami.Units.iconSizes.sizeForLabels
					implicitHeight: Kirigami.Units.iconSizes.sizeForLabels
					anchors {
						left: parent.left
						topMargin: Kirigami.Units.smallSpacing
						bottomMargin: Kirigami.Units.smallSpacing
						leftMargin: Kirigami.Units.smallSpacing * 2
						verticalCenter: parent.verticalCenter
						verticalCenterOffset: Math.round((parent.topPadding - parent.bottomPadding) / 2)
					}

					Controls.BusyIndicator {
						running: parent.visible
						anchors.fill: parent
					}
				}

				Behavior on opacity {
					NumberAnimation {}
				}
			}
		}
	}
}
