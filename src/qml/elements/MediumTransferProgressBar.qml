// SPDX-FileCopyrightText: 2026 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

/**
 * Progress bar used for uploading/downloading a medium.
 */
CircleProgressBar {
	id: root

	property var file
	property int deliveryState
	readonly property bool fileUploadNeeded: deliveryState === Enums.DeliveryState.Pending && file.transferState !== File.TransferState.Done
	property bool shown: file.transferState === File.TransferState.Transferring || !file.locallyAvailable || fileUploadNeeded

	value: transferWatcher.progress
	// Do not apply the opacity to child items.
	layer.enabled: true

	Behavior on opacity {
		NumberAnimation {}
	}

	Kirigami.Icon {
		source: {
			if (root.file.transferState === File.TransferState.Pending || root.file.transferState === File.TransferState.Transferring) {
				return "content-loading-symbolic"
			}

			if (root.fileUploadNeeded) {
				return "view-refresh-symbolic"
			}

			if (!root.file.locallyAvailable) {
				return "folder-download-symbolic"
			}
		}
		color: root.file.transferState === File.TransferState.Transferring ? Kirigami.Theme.activeTextColor : Kirigami.Theme.textColor
		isMask: true
		anchors.centerIn: parent
	}

	FileProgressWatcher {
		id: transferWatcher
		fileId: root.file.fileId
	}
}
