// SPDX-FileCopyrightText: 2023 Mathis Brüchert <mbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.5 as Controls

import org.kde.kirigami 2.5 as Kirigami

Item {
	property string title

	property real topPadding: 0
	property real bottomPadding: 0
	property real leftPadding: 0
	property real rightPadding: 0

	Layout.fillHeight: true
	Layout.fillWidth: true
}