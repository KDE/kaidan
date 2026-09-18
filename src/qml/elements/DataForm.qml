// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan

ColumnLayout {
	id: root

	property alias model: repeater.model
	property bool displayTitle: false
	property bool displayInstructions: false
	property var lastTextFieldAcceptedFunction

	spacing: 0

	FormCard.FormTextDelegate {
		visible: displayTitle
		text: root.displayTitle && root.model && root.model.sourceModel ? root.model.sourceModel.title : ""
		description: root.displayInstructions && root.model && root.model.sourceModel ? root.model.sourceModel.instructions : ""
	}

	Repeater {
		id: repeater

		Loader {
			sourceComponent: model.type === DataFormModel.HiddenField ? undefined : content
			Layout.fillWidth: true

			Component {
				id: content

				ColumnLayout {
					spacing: 0

					Loader {
						sourceComponent: model.mediaUrl.toString() ? image : undefined
						Layout.alignment: Qt.AlignHCenter
						Layout.fillWidth: true

						Component {
							id: image

							FormCard.AbstractFormDelegate {
								focusPolicy: Qt.NoFocus
								background: null
								contentItem: Image {
									source: model.mediaUrl
									fillMode: Image.PreserveAspectFit
								}
							}
						}
					}

					Loader {
						sourceComponent: model.isRequired && (model.type === DataFormModel.TextSingleField || model.type === DataFormModel.TextPrivateField) ? textField : text
						Layout.fillWidth: true

						Component {
							id: textField

							FormCard.FormTextFieldDelegate {
								label: model.label
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
						}

						Component {
							id: text

							ColumnLayout {
								spacing: 0

								FormCard.FormTextDelegate {
									description: model.label
								}

								FormCard.AbstractFormDelegate {
									background: null
									contentItem: FormattedTextEdit {
										text: model.value
										enhancedFormatting: true
									}
								}
							}
						}
					}
				}
			}
		}
	}
}
