// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami

/**
 * Image with rounded corners.
 */
Kirigami.Icon {
	id: root

	property real radius: relativeRoundedCornersRadius(width, height)

	layer.enabled: true
	layer.effect: Kirigami.ShadowedTexture {
		radius: root.radius
	}
}
