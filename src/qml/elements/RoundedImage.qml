// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Effects
import org.kde.kirigami as Kirigami

/**
 * Image with rounded corners.
 */
Kirigami.Icon {
	id: root

	property real radius: relativeRoundedCornersRadius(width, height)

	layer.enabled: true
	layer.effect: OpacityMask {
		maskSource: ShaderEffectSource {
			sourceItem: Item {
				width: root.paintedWidth
				height: root.paintedHeight

				Rectangle {
					radius: root.radius
					width: root.width
					height: root.height
					anchors.centerIn: parent
				}
			}
		}
	}
}
