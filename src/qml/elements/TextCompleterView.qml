// SPDX-FileCopyrightText: 2020 Matthias Ansorg
// SPDX-FileCopyrightText: 2026 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// Autocomplete dropdown.
//   Using Rectangle{Column{Repeater}} here because a ListView does not support setting its
//   height to its content's height because it's meant to be scrolled. Doesn't work even
//   with "height: childrenRect.height". The Rectangle is just to provide a background canvas.
//
//   TODO: Make this a sub-component, means provide a public interface of (alias) properties
//   and signals that is then accessed by the rest of the AutoComplete code. That avoids
//   the confusing parallel use of "root" and "completions".
ListView {
	id: root

	// The TextInput like control
	required property Item textControl

	// The model role name to query
	property string role: "modelData"

	// model: Data source containing the current autocomplete suggestions.
	//   This property must be set when instantiating this QML component. Acceptable models are string
	//   list models only. In JavaScript, these are a string array, so access it with [index], not the usual
	//   .get(index). To implement such a model in C++, use QStringList.
	//
	//   TODO: Allow client code of this component to set this property to any type of model accepted by a
	//   Repeater. See: https://doc.qt.io/qt-5/qml-qtquick-repeater.html#model-prop . Right now, only a
	//   QStringList is accepted.

	// The currently active, in-use user input that is the basis for the current completions.
	//   It differs from the text field's "text" content while navigating through autocompletions.
	//   During that time, the last input as typed by the user is "input" and the text field content is
	//   a potential next input, with completion highlighting done based on the current input.
	//
	//   Use the `onInputChanged` handler to update the model providing the completions to the
	//   autocomplete. For example, if the model is the result of a database query, then the database
	//   query has to be run again to update it.
	//
	//   TODO: If it is necessary to also keep the "text" property, better rename the "input" property
	//   to "searchTerm". Because it's confusing otherwise.
	property string input

	// The number of maximum visible search results.
	// If there are more results, a scrollbar will appear to scroll through all of them.
	property int maximumVisibleResultCount: 3

	// If the box with completions is visible below the autocomplete field.
	property bool completionsVisible: false // Will be made visible once starting to type a category name.

	// Normalize user input to a form based on which the completions can be calculated.
	//   Only a do-nothing implementation is provided. Client code should overwrite this as required
	//   by the model used.
	property var normalize: function(input) {
		return input
	}

	implicitHeight: count > 0 ? contentHeight / count * Math.min(count, maximumVisibleResultCount) : 0
	implicitWidth: textControl.width
	currentIndex: -1 // No element highlighted initially.
	clip: true

	ScrollBar.vertical: ScrollBar {
	}

	// A delegate renders one list item.
	//   TODO: Use a basic QML component to not be tied to Kirigami. Or document what
	//   can be used here when wanting to use it independent of Kirigami.
	delegate: ItemDelegate {
		readonly property string value: model[root.role]
		readonly property real verticalScrollBarWidth: ListView.view.ScrollBar.vertical.visible ? ListView.view.ScrollBar.vertical.width : 0

		width: ListView.view.width - verticalScrollBarWidth
		text: highlightCompletion(value, root.input)

		// Background coloring should be used only for the selected item.
		//   (Also, a lighter colored background automatically appears on mouse-over.)
		highlighted: model.index !== -1 && model.index === root.currentIndex

		onHighlightedChanged: {
			if (highlighted) {
				//console.log( "completions.model[" + model.index + "]: " + JSON.stringify(value))
				root.textControl.text = value
			}
		}

		onClicked: {
			//console.log("modelData = " + JSON.stringify(value))
			root.currentIndex = model.index
			root.textControl.accepted()
		}

		// Highlight the auto-completed parts of a multi-substring search term using HTML.
		// This implementation uses <b> tags to highlight the completed portions. Client code can
		// overwrite this to implement a different type of completion.
		// @param fragmentsString  The part to not render in bold, when matched case-insensitively against the
		//   completion.
		//
		// TODO: Document the parameters.
		// TODO: Make sure original contains no HTML tags by sanitizing these. Otherwise searching
		//   for parts of original below may match "word</b>" etc. and mess up the result.
		// TODO: Add a parameter to make it configurable which HTML tag (incl. attributes) to use for the
		//   highlighting.
		// TODO: Maybe add a parameter to de-highlight the original parts with configurable HTML.
		// TODO: It seems better to do this in C++, as it will be much faster in regex processing etc..
		//   And it seems to be the responsibility of the model, and all models of this application are
		//   done in C++ as a convention.
		function highlightCompletion(completion, fragmentsString) {
			const fragments = fragmentsString.trim().split(" ")

			// Tracks the current processing position in "completion".
			//   This ensures that fragments are found in their given sequence. It also prevents
			//   modifying the <b> HTML tags themselves by matching against a "b" fragment. We'll simply
			//   work only on the part that has not yet had any insertions of HTML tags.
			let searchPos = 0

			// De-bold the sequential first occurrences of the fragments.
			for (let i = 0; i < fragments.length; i++) {
				const iCompletion = completion.toLowerCase() // "i" as in "case-Insensitive"
				const fragment = fragments[i]
				const fragmentStart = iCompletion.indexOf(fragment.toLowerCase(),
														  searchPos)

				// Nothing to de-bolden if fragment was not found.
				//   TODO: This is an error condition that should generate a warning.
				if (fragmentStart === -1)
					continue

				const fragmentEnd = fragmentStart + fragment.length
				const completionBefore = completion.substr(0, fragmentStart)
				const completionDebold = completion.substr(fragmentStart, fragment.length)
				const completionAfter = completion.substr(fragmentEnd)

				completion = completionBefore + "</b>" + completionDebold + "<b>" + completionAfter

				// The search for the next fragment may start with completionAfter. That's after all HTML
				// tags inserted so far, which prevents messing them up when a "b" fragment matches against them.
				searchPos = completion.length - completionAfter.length - 1
			}

			// Wrap everything in bold to provide the proper "context" for the de-boldening above.
			//   This could not be done before to prevent matches of a "b" fragment against </b>".
			completion = "<b>" + completion + "</b>"

			return completion
		}
	}

	Connections {
		target: root.textControl

		// This event handler is undocumented for TextField and incompletely documented for TextInput,
		// which TextField wraps: https://doc.qt.io/qt-5/qml-qtquick-textinput.html#textEdited-signal .
		// However, it works, and is also proposed by code insight in Qt Creator.
		function onTextEdited() {
			// TODO: Remove Qt.callLater scope once Kirigami Addons > v1.12.1 is required by Kaidan.
			Qt.callLater(function() {
				// Update the current input because the user changed the text.
				//   User changes include cutting and pasting. The "textChanged()" event however
				//   is emitted also when software changes the text field content (such as when
				//   navigating through completions), making it the wrong place to update the input.
				//   This automatically emits inputChanged() so client code can adapt the model.
				root.input = root.textControl.text

				// Invalidate the completion selection, because the user edited the input so
				// it does not correspond to any current completion. Also completions might have been cleared.
				//   TODO: Probably better implement this reactively via onModelChanged, if there is such a thing.
				root.currentIndex = -1

				root.completionsVisible = root.count > 0 ? true : false
			});
		}

		// Handle the "text accepted" event, which sets the input from the text.
		//   This event is also artificially emitted by the "Search" button and by clicking on
		//   an auto-suggest proposal.
		//
		//   This event is emitted by a TextField when the user finishes editing it.
		//   In desktop software, this requires pressing "Return". Moving focus does not count.
		function onAccepted() {
			// TODO: Remove Qt.callLater scope once Kirigami Addons > v1.12.1 is required by Kaidan.
			Qt.callLater(function() {
				//console.log("AutoComplete: field: 'accepted()' received")

				// Give up the focus, making it available for grabs by the rest of the UI
				// via appropriate focus property bindings.
				root.textControl.focus = false

				root.completionsVisible = false
				// When clicking into the text field again, the last set of completions should show
				// again. But selecting them will start anew.
				//   TODO: Probably better implement this reactively via onModelChanged, if there is such a thing.
				root.currentIndex = -1

				// True to the browser paradigm where URLs are fixed up, we'll correct the input entered.
				// Such as: " 2 165741  004149  " → "2165741004149"
				const searchTerm = root.normalize(root.textControl.text)
				root.input = searchTerm
			});
		}

		function onActiveFocusChanged() {
			// TODO: Remove Qt.callLater scope once Kirigami Addons > v1.12.1 is required by Kaidan.
			Qt.callLater(function() {
				if (root.textControl.activeFocus && root.count > 0)
					root.completionsVisible = (root.textControl.text === "" || root.textControl.text.match("^[0-9 ]+$")) ? false : true
				// TODO: Probably better use "input" instead of "text" in the line above.
				// TODO: Perhaps initialize the completions with suggestions based on the current
				// text. If the reason for not having the focus before was a previous
				// search, then it has no completions at this point.
				else
					root.completionsVisible = false
			});
		}
	}

	Connections {
		target: root.textControl.Keys

		// Process all keyboard events here centrally.
		//   Since the TextEdit plus suggestions box is one combined component, handling all
		//   key presses here is more tidy. They cannot all be handled in root as
		//   key presses are not delivered or forwarded to components in their invisible state.
		function onPressed(event) {
			//console.log("AutoComplete: field: Keys.pressed(): " + event.key + " : " + event.text)

			if (root.completionsVisible) {
				switch (event.key) {
				case Qt.Key_Escape:
					root.completionsVisible = false
					root.currentIndex = -1
					event.accepted = true
					break

				case Qt.Key_Up:
					// When moving prior the first item, cycle through completions from the end again.
					if (root.currentIndex === 0) {
						root.currentIndex = root.count - 1
					} else {
						root.currentIndex--
					}
					event.accepted = true
					break

				case Qt.Key_Down:
					// When moving past the last item, cycle through completions from the start again.
					if (root.currentIndex === root.count - 1) {
						root.currentIndex = 0
					} else {
						root.currentIndex++
					}
					event.accepted = true
					break

				case Qt.Key_Return:
					if (root.currentIndex >= 0) {
						root.textControl.accepted()
					}
					event.accepted = true
					break
				}
			} else {
				switch (event.key) {
					// This way, "double Escape" can be used to move the focus to the browser.
					// The first hides the suggestions box, the second moves the focus.
				case Qt.Key_Escape:
					// Give up the focus, making it available for grabs by the rest of the UI
					// via appropriate focus property bindings.
					root.textControl.focus = false
					event.accepted = true
					break

				case Qt.Key_Down:
					root.completionsVisible = root.count > 0 ? true : false
					event.accepted = true
					break
				}
			}
		}
	}

	// React to our own auto-provided signal for a change in the "input" property.
	//   When client code also implements onInputChanged when instantiating an this, it
	//   will not overwrite this handler but add to it. So no caveats when reacting to own signals.
	//
	//   TODO: Make sure that text is only set here and not also unnecessarily in other locations in
	//   this component.
	onInputChanged: {
		//console.log("TextFieldCompleter: 'inputChanged()' signal received")
		textControl.text = input
	}

	// Event handler for the non-instantiable QML object "Qt" resp. "Qt.inputMethod".
	//   The details of detecting keyboard changes like this: https://stackoverflow.com/a/62986064
	Connections {
		target: Qt.inputMethod

		// Hide the autocomplete when the user hides the Android keyboard.
		//   Because it probably means that the user wants to see the browser, not the
		//   completions box. So we hide the completions box by removing the focus. We could
		//   also keep the focus, but focus is useless without a keyboard.
		function onKeyboardRectangleChanged() {
			// Contains QML Basic Type "rect", automatically converted from Qt QRectF as per
			// https://doc.qt.io/qt-5/qml-rect.html
			const newRect = Qt.inputMethod.keyboardRectangle

			if (newRect.height === 0) {
				root.textControl.focus = false
			}
		}
	}
}
