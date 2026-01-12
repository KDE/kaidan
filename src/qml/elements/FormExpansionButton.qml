// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * Button used to expand and collapse form entries above it.
 */
FormIconButton {
	id: root
	checkable: true
	icon.name: root.checked ? "go-up-symbolic" : "go-down-symbolic"
}
