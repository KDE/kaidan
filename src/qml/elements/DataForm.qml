// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

ColumnLayout {
	id: root

	property alias model: repeater.model
	property bool displayTitle: true
	property bool displayInstructions: true
	property var lastTextFieldAcceptedFunction

	Kirigami.Heading {
		visible: displayTitle
		text: root.model && root.model.sourceModel ? root.model.sourceModel.title : ""
		textFormat: Text.PlainText
		wrapMode: Text.WordWrap
	}

	Controls.Label {
		visible: displayInstructions
		text: root.model && root.model.sourceModel ? root.model.sourceModel.instructions : ""
		textFormat: Text.PlainText
		wrapMode: Text.WordWrap
	}

	Kirigami.FormLayout {
		Repeater {
			id: repeater
			delegate: Column {
				visible: model.type !== DataFormModel.HiddenField
				Kirigami.FormData.label: model.label

				Loader {
					id: imageLoader
				}

				Kirigami.ActionTextField {
					id: textField
					visible: model.isRequired && (model.type === DataFormModel.TextSingleField || model.type === DataFormModel.TextPrivateField)
					echoMode: model.type === DataFormModel.TextPrivateField ? TextInput.Password : TextInput.Normal
					onTextChanged: model.value = text
					onAccepted: {
						if (index === repeater.count - 1) {
							lastTextFieldAcceptedFunction()
						} else {
							nextItemInFocusChain().forceActiveFocus()
						}
					}

					Component.onCompleted: text = model.value
				}

				FormattedTextEdit {
					visible: !textField.visible
					text: model.value
				}

				Component {
					id: imageComponent

					Image {
						source: model.mediaUrl
					}
				}

				Component.onCompleted: {
					if (model.mediaUrl) {
						imageLoader.sourceComponent = imageComponent
					}
				}
			}
		}
	}
}
