// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Layouts 1.14

DetailsSheet {
	id: root

	required property string jid

	parent: applicationWindow().overlay

	ContactDetailsHeader {
		jid: root.jid
	}

	ContactDetailsContent {
		sheet: root
		jid: root.jid
		Layout.fillWidth: true
	}
}
