// SPDX-FileCopyrightText: 2020 Matthias Ansorg
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// Autocomplete widget with completion highlighting similar to a web search.
//
//   When using this, be sure to adjust the z indexes of sibling items so that the container of the
//   autocomplete widget (e.g. RowLayout) has a higher z value than the container of every widget
//   following it that might be overlaid by the suggestions.
//
//   TODO: Set a minimum width. Otherwise, without "Layout.fillWidth: true", the autocomplete box
//   would not be visible at all.
//
//   TODO: Fix that the parts of this auto-complete widget have confusing internal
//   dependencies. For example, after changing the input property one also always has to
//   do completions.currentIndex = -1. That should be handled automatically.
//
//   TODO: Clear up the confusion between an invisible suggestion box and a visible one with
//   no content. Both cases are used currently, but only one is necessary, as both look the same.
//
//   TODO: Use custom properties as a more declarative way to govern behavior here.
//   For example, "completionsVisibility" set to an expression. See:
//   https://github.com/dant3/qmlcompletionbox/blob/41eebf2b50ef4ade26c99946eaa36a7bfabafef5/SuggestionBox.qml#L36
//   https://github.com/dant3/qmlcompletionbox/blob/master/SuggestionBox.qml
//
//   TODO: Use states to better describe the open and closed state of the completions box.
//   See: https://code.qt.io/cgit/qt/qtdeclarative.git/tree/examples/quick/keyinteraction/focus/focus.qml?h=5.15#n166
Popup {
	id: root

	property alias textControl: completions.textControl
	property alias role: completions.role
	property alias model: completions.model
	property alias maximumVisibleResultCount: completions.maximumVisibleResultCount
	property alias normalize: completions.normalize
	readonly property alias input: completions.input

	margins: 0
	padding: 0
	parent: textControl
	y: textControl.height
	closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
	visible: completions.completionsVisible
	background: Rectangle {
		color: Kirigami.Theme.backgroundColor
		border {
			width: 1
			color: Kirigami.Theme.textColor
		}
	}
	contentItem: TextCompleterView {
		id: completions
		textControl: root.textControl
	}

	// Custom mouse event handler that will show the completions box when clicking into the
	// text field. It is the complementary action to pressing "Esc" once, and does the same as
	// pressing "Arrow Down" while root is hidden.
	//
	//   Due to a bug in Qt, "propagateComposedEvents: true" has no effect when used inside a
	//   TextField in a StackView page (see https://forum.qt.io/topic/64041 ). There is a
	//   workaround in C++, but it is complex (see https://forum.qt.io/post/312884 ). Since this
	//   is non-essential behavior, we better watit for Qt to fix the bug.
	//
	//   TODO: Enable this code once Qt fixed the bug described above.
//	MouseArea {
//		anchors.fill: parent
//		// By propagating events and not accepting them in the handlers, the parent TextEdit can
//		// also react to them to set focus etc.. Source: https://stackoverflow.com/a/29765628
//		// propagateComposedEvents: true
//
//		onClicked: {
//			//console.log("AutoComplete: field: clicked() received")
//			completions.completionsVisible = completions.count > 0 ? true : false
//			mouse.accepted = false
//		}
//		// onPressed: mouse.accepted = false
//		onDoubleClicked: mouse.accepted = false
//		onPressAndHold: mouse.accepted = false
//	}
}
