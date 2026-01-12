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
// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Material as Material
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

import "details"
import "elements"
import "registration"

Kirigami.ApplicationWindow {
	id: root

	property bool quitForced: false
	readonly property color primaryBackgroundColor: Kirigami.Theme.backgroundColor
	readonly property color secondaryBackgroundColor: {
		const accentColor = Kirigami.Theme.textColor
		return Qt.tint(primaryBackgroundColor, Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.05))
	}
	readonly property color tertiaryBackgroundColor: {
		const accentColor = secondaryBackgroundColor
		return Qt.tint(primaryBackgroundColor, Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.8))
	}
	readonly property real reducedBackgroundOpacity: 0.4
	// radius for using rounded corners
	readonly property int roundedCornersRadius: Kirigami.Units.smallSpacing * 1.5
	readonly property int largeButtonWidth: Kirigami.Units.gridUnit * 25
	readonly property int smallButtonWidth: Kirigami.Theme.defaultFont.pixelSize * 2.9

	minimumHeight: 300
	minimumWidth: 300
	pageStack.globalToolBar {
		showNavigationButtons: pageStack.wideMode || pageStack.currentIndex === 0 ? Kirigami.ApplicationHeaderStyle.NoNavigationButtons : Kirigami.ApplicationHeaderStyle.ShowBackButton
		preferredHeight: Kirigami.Units.iconSizes.large + Kirigami.Units.smallSpacing * 3
	}
	globalDrawer: GlobalDrawer {}
	onClosing: close => {
		// Disconnect from the server when the application window is closed but before the application
		// is quit.
		if (!quitForced) {
			if (AccountController.logOutAllAccountsToQuit()) {
				close.accepted = false
				forcedClosingTimer.start()
			}
		}
	}
	Component.onCompleted: {
		HostCompletionModel.rosterModel = RosterModel
		HostCompletionModel.aggregateKnownProviders()

		// Restore the latest application window state if it is stored.
		if (!Kirigami.Settings.isMobile) {
			const latestPosition = Settings.windowPosition
			root.x = latestPosition.x
			root.y = latestPosition.y

			const latestSize = Settings.windowSize
			if (latestSize.width > 0) {
				root.width = latestSize.width
				root.height = latestSize.height
			} else {
				root.width = Kirigami.Units.gridUnit * 85
				root.height = Kirigami.Units.gridUnit * 55
			}
		}
	}
	Component.onDestruction: {
		// Store the application window state for restoring the latest state on the next start.
		if (!Kirigami.Settings.isMobile) {
			Settings.windowPosition = Qt.point(x, y)
			Settings.windowSize = Qt.size(width, height)
		}
	}

	StatusBar {
		color: Material.Material.color(Material.Material.Green, Material.Material.Shade700)
	}

	Component {
		id: startPage

		StartPage {}
	}

	Component {
		id: rosterPage

		RosterPage {}
	}

	Component {
		id: emptyChatPage

		EmptyChatPage {}
	}

	Component {
		id: callPage

		CallPage {}
	}

	// Forces closing the application if the client does not disconnect from the server.
	// That can happen if there are connection problems.
	Timer {
		id: forcedClosingTimer
		interval: 5000
		onTriggered: {
			quitForced = true
			close()
		}
	}

	Connections {
		target: MainController

		function onRaiseWindowRequested() {
			if (!root.active) {
				root.raise()
				root.requestActivate()
			}
		}

		function onPassiveNotificationRequested(text, duration) {
			if (duration.length) {
				showPassiveNotification(text, duration)
			} else {
				passiveNotification(text)
			}
		}

		function onOpenStartPageRequested() {
			openStartPage(true)
		}

		function onActiveCallChanged() {
			if (MainController.activeCall) {
				openPage(callPage)
			}
		}
	}

	Connections {
		target: AccountController

		function onAccountAvailable() {
			openChatView()
		}

		function onNoAccountAvailable() {
			openStartPage()
		}

		function onAccountAdded(account) {
			openChatView()
		}

		function onAllAccountsLoggedOutToQuit() {
			root.close()
		}
	}

	Connections {
		target: pageStack

		function onWideModeChanged() {
			showProperPageForNarrorWindow()
		}
	}

	Binding {
		target: ImageProvider
		property: "screenDevicePixelRatio"
		value: Screen.devicePixelRatio
	}

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

	function openStartPage(accountAvailable = false) {
		if (accountAvailable) {
			openPage(startPage)
		} else {
			popLayersAboveLowest()
			popAllPages()
			pageStack.push(startPage)
		}
	}

	/**
	 * Opens the view with the roster and chat page.
	 */
	function openChatView() {
		popLayersAboveLowest()
		popAllPages()
		pageStack.push(rosterPage)
		resetChatView()
	}

	function resetChatView() {
		if (!Kirigami.Settings.isMobile) {
			pageStack.push(emptyChatPage)
		}

		showProperPageForNarrorWindow()
	}

	/**
	 * Creates and opens an overlay (e.g., Kirigami.OverlaySheet or Kirigami.Dialog) on desktop
	 * devices or a page (e.g., Kirigami.ScrollablePage) on mobile devices.
	 *
	 * @param overlayComponent component containing the overlay to be opened
	 * @param pageComponent component containing the page to be opened
	 *
	 * @return the opened overlay or page
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
		return pushLayer(pageComponent)
	}

	function showProperPageForNarrorWindow() {
		if (!pageStack.wideMode && pageStack.layers.depth === 1) {
			if (pageStack.currentItem instanceof EmptyChatPage) {
				pageStack.goBack()
			} else if (pageStack.lastItem instanceof ChatPage && pageStack.currentItem instanceof RosterPage) {
				pageStack.goForward()
			}
		}
	}

	function pushLayer(layer) {
		return pageStack.layers.push(layer)
	}

	function popLayer() {
		pageStack.layers.pop()
	}

	/**
	 * Pops a given count of layers from the page stack.
	 *
	 * @param countOfLayersToPop count of layers which are popped
	 */
	function popLayers(countOfLayersToPop) {
		for (let i = 0; i < countOfLayersToPop; i++)
			popLayer()
	}

	/**
	 * Pops all layers except the layer with index 0 from the page stack.
	 */
	function popLayersAboveLowest() {
		while (pageStack.layers.depth > 1)
			popLayer()
	}

	/**
	 * Pops all pages from the page stack.
	 */
	function popAllPages() {
		while (pageStack.depth > 0)
			pageStack.pop()
	}
}
