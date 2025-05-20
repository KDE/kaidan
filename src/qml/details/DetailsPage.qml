// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import im.kaidan.kaidan

import "../elements"

FormInfoPage {
	id: root

	property Account account

	DisabledAccountHint {
		account: root.account
	}
}
