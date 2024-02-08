// SPDX-FileCopyrightText: 2016 geobra <s.g.b@gmx.de>
// SPDX-FileCopyrightText: 2016 Marzanna <MRZA-MRZA@users.noreply.github.com>
// SPDX-FileCopyrightText: 2016 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2016 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2017 Ilya Bizyaev <bizyaev@zoho.com>
// SPDX-FileCopyrightText: 2018 Nicolas Fella <nicolas.fella@gmx.de>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Bhavy Airi <airiraghav@gmail.com>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2023 Mathis Brüchert <mbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls.Material 2.14 as Material
import org.kde.kirigami 2.19 as Kirigami
import StatusBar 0.1

import im.kaidan.kaidan 1.0

import "details"
import "elements"
import "registration"
import "settings"

Kirigami.ApplicationWindow {
	id: root

	minimumHeight: 300
	minimumWidth: 280

	readonly property color primaryBackgroundColor: {
		Kirigami.Theme.colorSet = Kirigami.Theme.View
		return Kirigami.Theme.backgroundColor
	}

	readonly property color secondaryBackgroundColor: {
		Kirigami.Theme.colorSet = Kirigami.Theme.Window
		return Kirigami.Theme.backgroundColor
	}

	readonly property color tertiaryBackgroundColor: {
		const accentColor = secondaryBackgroundColor
		return Qt.tint(primaryBackgroundColor, Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.7))
	}

	// radius for using rounded corners
	readonly property int roundedCornersRadius: Kirigami.Units.smallSpacing * 1.5

	readonly property int largeButtonWidth: Kirigami.Units.gridUnit * 25
	readonly property int smallButtonWidth: Kirigami.Theme.defaultFont.pixelSize * 2.9

	// This is an alias for use in settings ONLY
	// it is only used on mobile, on desktop another item overrides the id "stack"
	property var stack: SettingsStack {}

	StatusBar {
		color: Material.Material.color(Material.Material.Green, Material.Material.Shade700)
	}

	// Global and Contextual Drawers
	// It is initialized as invisible.
	// That way, it does not pop up for a moment before the startPage is opened.
	globalDrawer: GlobalDrawer {
		enabled: false
	}

	contextDrawer: Kirigami.ContextDrawer {
		id: contextDrawer
	}

	// Needed to be outside of the DetailsSheet to not be destroyed with it.
	// Otherwise, the undo action of "showPassiveNotification()" would point to a destroyed object.
	BlockingAction {
		id: blockingAction
		onSucceeded: (jid, block) => {
			// Show a passive notification when a JID that is not in the roster is blocked and
			// provide an option to undo that.
			// JIDs in the roster can be blocked again via their details.
			if (!block && !RosterModel.hasItem(jid)) {
				showPassiveNotification(qsTr("Unblocked %1").arg(jid), "long", qsTr("Undo"), () => {
					blockingAction.block(jid)
				})
			}
		}
		onErrorOccurred: (jid, block, errorText) => {
			if (block) {
				showPassiveNotification(qsTr("Could not block %1: %2").arg(jid).arg(errorText))
			} else {
				showPassiveNotification(qsTr("Could not unblock %1: %2").arg(jid).arg(errorText))
			}
		}
	}

	// components for all main pages
	Component {id: startPage; StartPage {}}
	Component {id: automaticRegistrationPage; AutomaticRegistrationPage {}}
	Component {id: manualRegistrationPage; ManualRegistrationPage {}}
	Component {id: rosterPage; RosterPage {}}
	Component {id: chatPage; ChatPage {}}
	Component {id: emptyChatPage; EmptyChatPage {}}
	Component {id: settingsPage; SettingsPage {}}
	Component {id: accountDetailsSheet; AccountDetailsSheet {}}
	Component {id: accountDetailsPage; AccountDetailsPage {}}
	Component {id: avatarChangePage; AvatarChangePage {}}
	Component {id: qrCodeOnboardingPage; QrCodeOnboardingPage {}}
	Component {id: contactAdditionPage; ContactAdditionPage {}}
	Component {id: contactAdditionDialog; ContactAdditionDialog {}}

	Component {
		id: accountDetailsKeyAuthenticationPage

		KeyAuthenticationPage {
			Component.onDestruction: openView(accountDetailsSheet, accountDetailsPage)
		}
	}

	onWideScreenChanged: showRosterPageForNarrowWindow()

	/**
	 * Returns a radius used for rectangles with rounded corners that is relative to the
	 * rectangle's dimensions.
	 *
	 * @param width width of the rectangle
	 * @param height height of the rectangle
	 */
	function relativeRoundedCornersRadius(width, height) {
		return Math.sqrt(width < height ? width : height)
	}

	/**
	 * Shows a passive notification for a long period.
	 */
	function passiveNotification(text) {
		showPassiveNotification(text, "long")
	}

	function openStartPage() {
		globalDrawer.enabled = false

		popLayersAboveLowest()
		popAllPages()
		pageStack.push(startPage)
	}

	/**
	 * Opens the view with the roster and chat page.
	 */
	function openChatView() {
		globalDrawer.enabled = true

		popLayersAboveLowest()
		popAllPages()
		pageStack.push(rosterPage)
		resetChatView()
	}

	function resetChatView() {
		if (!Kirigami.Settings.isMobile) {
			pageStack.push(emptyChatPage)
		}

		showRosterPageForNarrowWindow()
	}

	/**
	 * Creates and opens an overlay (e.g., Kirigami.OverlaySheet or Kirigami.Dialog) on desktop
	 * devices or a page (e.g., Kirigami.ScrollablePage) on mobile devices.
	 *
	 * @param overlayComponent component containing the overlay to be opened
	 * @param pageComponent component containing the page to be opened
	 *
	 * @return the opened page or sheet
	 */
	function openView(overlayComponent, pageComponent) {
		if (Kirigami.Settings.isMobile) {
			return openPage(pageComponent)
		} else {
			return openOverlay(overlayComponent)
		}
	}

	function openOverlay(overlayComponent) {
		let overlay = overlayComponent.createObject(root)
		overlay.open()
		return overlay
	}

	function openPage(pageComponent) {
		popLayersAboveLowest()
		return pageStack.layers.push(pageComponent)
	}

	// Show the rosterPage instead of the emptyChatPage if the window is narrow.
	function showRosterPageForNarrowWindow() {
		if (pageStack.layers.depth < 2 && pageStack.currentItem instanceof EmptyChatPage && !wideScreen)
			pageStack.goBack()
	}

	/**
	 * Pops a given count of layers from the page stack.
	 *
	 * @param countOfLayersToPop count of layers which are popped
	 */
	function popLayers(countOfLayersToPop) {
		for (let i = 0; i < countOfLayersToPop; i++)
			pageStack.layers.pop()
	}

	/**
	 * Pops all layers except the layer with index 0 from the page stack.
	 */
	function popLayersAboveLowest() {
		while (pageStack.layers.depth > 1)
			pageStack.layers.pop()
	}

	/**
	 * Pops all pages from the page stack.
	 */
	function popAllPages() {
		while (pageStack.depth > 0)
			pageStack.pop()
	}

	Connections {
		target: Kaidan

		function onRaiseWindowRequested() {
			if (!root.active) {
				root.raise()
				root.requestActivate()
			}
		}

		function onPassiveNotificationRequested(text) {
			passiveNotification(text)
		}

		function onCredentialsNeeded() {
			openStartPage()
		}

		function onOpenChatViewRequested() {
			openChatView()
		}
	}

	Component.onCompleted: {
		HostCompletionModel.rosterModel = RosterModel;
		HostCompletionModel.aggregateKnownProviders();

		// Restore the latest application window state if it is stored.
		if (!Kirigami.Settings.isMobile) {
			const latestPosition = Kaidan.settings.windowPosition
			root.x = latestPosition.x
			root.y = latestPosition.y

			const latestSize = Kaidan.settings.windowSize
			if (latestSize.width > 0) {
				root.width = latestSize.width
				root.height = latestSize.height
			}
		}

		if (AccountManager.loadConnectionData()) {
			openChatView()
			// Announce that the user interface is ready and the application can start connecting.
			Kaidan.logIn()
		} else {
			openStartPage()
		}
	}

	Component.onDestruction: {
		// Store the application window state for restoring the latest state on the next start.
		if (!Kirigami.Settings.isMobile) {
			Kaidan.settings.windowPosition = Qt.point(x, y)
			Kaidan.settings.windowSize = Qt.size(width, height)
		}
	}
}
