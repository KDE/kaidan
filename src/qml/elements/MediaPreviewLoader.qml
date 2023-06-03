// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls
import QtPositioning 5.14 as Positioning

import im.kaidan.kaidan 1.0
import MediaUtils 0.1

Loader {
	id: root

	property url mediaSource
	property int mediaSourceType
	property bool showOpenButton
	property QtObject message
	property QtObject mediaSheet

	enabled:  {
		switch (mediaSourceType) {
		case Enums.MessageType.MessageUnknown:
		case Enums.MessageType.MessageText:
			return false
		case Enums.MessageType.MessageImage:
		case Enums.MessageType.MessageAudio:
		case Enums.MessageType.MessageVideo:
		case Enums.MessageType.MessageFile:
		case Enums.MessageType.MessageDocument:
		case Enums.MessageType.MessageGeoLocation:
			return mediaSource != '' && sourceComponent !== null
		}
	}
	visible: enabled
	sourceComponent: {
		switch (mediaSourceType) {
		case Enums.MessageType.MessageUnknown:
		case Enums.MessageType.MessageText:
			return null
		case Enums.MessageType.MessageImage:
			return imagePreview
		case Enums.MessageType.MessageAudio:
			return audioPreview
		case Enums.MessageType.MessageVideo:
			return videoPreview
		case Enums.MessageType.MessageFile:
		case Enums.MessageType.MessageDocument:
			return otherPreview
		case Enums.MessageType.MessageGeoLocation:
			return locationPreview
		}
	}

	Layout.fillHeight: item ? item.Layout.fillHeight : false
	Layout.fillWidth: item ? item.Layout.fillWidth : false
	Layout.preferredHeight: item ? item.Layout.preferredHeight : -1
	Layout.preferredWidth: item ? item.Layout.preferredWidth : -1
	Layout.minimumHeight: item ? item.Layout.minimumHeight : -1
	Layout.minimumWidth: item ? item.Layout.minimumWidth : -1
	Layout.maximumHeight: item ? item.Layout.maximumHeight : -1
	Layout.maximumWidth: item ? item.Layout.maximumWidth : -1
	Layout.alignment: item ? item.Layout.alignment : Qt.AlignCenter
	Layout.margins: item ? item.Layout.margins : 0
	Layout.leftMargin: item ? item.Layout.leftMargin : 0
	Layout.topMargin: item ? item.Layout.topMargin : 0
	Layout.rightMargin: item ? item.Layout.rightMargin : 0
	Layout.bottomMargin: item ? item.Layout.bottomMargin : 0

	Component {
		id: imagePreview

		MediaPreviewImage {
			mediaSource: root.mediaSource
			mediaSourceType: root.mediaSourceType
			message: root.message
			mediaSheet: root.mediaSheet
		}
	}

	Component {
		id: audioPreview

		MediaPreviewAudio {
			mediaSource: root.mediaSource
			mediaSourceType: root.mediaSourceType
			message: root.message
			mediaSheet: root.mediaSheet
		}
	}

	Component {
		id: videoPreview

		MediaPreviewVideo {
			mediaSource: root.mediaSource
			mediaSourceType: root.mediaSourceType
			message: root.message
			mediaSheet: root.mediaSheet
		}
	}

	Component {
		id: otherPreview

		MediaPreviewOther {
			mediaSource: root.mediaSource
		}
	}

	Component {
		id: locationPreview

		MediaPreviewLocation {
			mediaSource: root.mediaSource
			mediaSourceType: root.mediaSourceType
			message: root.message
			mediaSheet: root.mediaSheet
		}
	}
}
