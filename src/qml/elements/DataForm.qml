// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

ColumnLayout {
	id: root

	property alias model: repeater.model
	property bool displayTitle: true
	property bool displayInstructions: true
	property var lastTextFieldAcceptedFunction

	Kirigami.Heading {
		visible: displayTitle
		text: repeater.model.sourceModel ? repeater.model.sourceModel.title : ""
		textFormat: Text.PlainText
		wrapMode: Text.WordWrap
	}

	Controls.Label {
		visible: displayInstructions
		text: repeater.model.sourceModel ? repeater.model.sourceModel.instructions : ""
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
