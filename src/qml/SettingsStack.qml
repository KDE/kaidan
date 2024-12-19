// SPDX-FileCopyrightText: 2023 Mathis Brüchert <mbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick

Item {
	function push(page) {
		pushLayer("settings/PageWrapper.qml", {
			"source": page
		})
	}

	function pop() {
		popLayer()
	}
}
