// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Separator {
	visible: !Kirigami.Settings.isMobile
	implicitHeight: Kirigami.Units.smallSpacing
	Layout.fillWidth: true
}
