// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

ColumnLayout {
	id: root

	property QtObject firstVisibleTextField
	property QtObject model

	// base model if model is a filter model
	property QtObject baseModel: model

	property bool displayTitle: true
	property bool displayInstructions: true

	Kirigami.Heading {
		visible: displayTitle
		text: baseModel ? baseModel.title : ""
		textFormat: Text.PlainText
		wrapMode: Text.WordWrap
	}

	Controls.Label {
		visible: displayInstructions
		text: baseModel ? baseModel.instructions : ""
		textFormat: Text.PlainText
		wrapMode: Text.WordWrap
	}

	Kirigami.FormLayout {
		Repeater {
			id: repeater
			model: root.model
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

					Component.onCompleted: {
						text = model.value

						if (!root.firstVisibleTextField && visible)
							root.firstVisibleTextField = textField
					}
				}

				Controls.Label {
					visible: !textField.visible
					text: Utils.formatMessage(model.value)
					textFormat: Text.StyledText
					wrapMode: Text.WordWrap
					onLinkActivated: Qt.openUrlExternally(link)
				}

				Component {
					id: imageComponent

					Image {
						source: model.mediaUrl
					}
				}

				Component.onCompleted: {
					if (model.mediaUrl)
						imageLoader.sourceComponent = imageComponent
				}
			}
		}
	}
}
