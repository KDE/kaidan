// SPDX-FileCopyrightText: 2023 Mathis Brüchert <mbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14

Item {
	function push(page) {
		pageStack.layers.push("settings/PageWrapper.qml", {
			"source": page
		})
	}

	function pop() {
		pageStack.layers.pop()
	}
}
