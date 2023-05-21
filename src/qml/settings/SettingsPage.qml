// SPDX-FileCopyrightText: 2018 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import org.kde.kirigami 2.19 as Kirigami

Kirigami.ScrollablePage {
	title: qsTr("Settings")

	topPadding: 0
	bottomPadding: 0
	leftPadding: 0
	rightPadding: 0

	SettingsContent {}
}
