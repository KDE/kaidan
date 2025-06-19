// SPDX-FileCopyrightText: 2021 Arun PK <arun.pk@holoplot.com>
// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Shapes
import org.kde.kirigami as Kirigami

Item {
	id: root

    property real startAngle: 0
	property real stopAngle: 360
    property real minValue: 0
	property real maxValue: 1
    property real value: 0
	property color backgroundColor: Kirigami.Theme.backgroundColor
	property color progressLineBackgroundColor: "transparent"
	property color progressLineColor: Kirigami.Theme.activeTextColor
	property int progressLineWidth: Kirigami.Units.smallSpacing
	property int penCapStyle: Qt.RoundCap

	implicitWidth: 200
	implicitHeight: 200

    QtObject {
		id: internal

		property real baseRadius: Math.min(root.width / 2, root.height / 2)
		property real radiusOffset: root.progressLineWidth / 2
    }

    Shape {
        id: shape
        anchors.fill: parent
        layer.enabled: true
        layer.samples: 8

        ShapePath {
			id: background
			fillColor: root.backgroundColor
			strokeColor: "transparent"
			capStyle: root.penCapStyle

            PathAngleArc {
				radiusX: internal.baseRadius - root.progressLineWidth
				radiusY: internal.baseRadius - root.progressLineWidth
				centerX: root.width / 2
				centerY: root.height / 2
                startAngle: 0
                sweepAngle: 360
            }
        }

        ShapePath {
			id: progressLineBackground
			fillColor: "transparent"
			strokeColor: root.progressLineBackgroundColor
			strokeWidth: root.progressLineWidth
			capStyle: root.penCapStyle

            PathAngleArc {
				radiusX: internal.baseRadius - internal.radiusOffset
				radiusY: internal.baseRadius - internal.radiusOffset
				centerX: root.width / 2
				centerY: root.height / 2
				startAngle: root.startAngle - 90
				sweepAngle: root.stopAngle
            }
        }

        ShapePath {
			id: progress
			fillColor: "transparent"
			strokeColor: root.progressLineColor
			strokeWidth: root.progressLineWidth
			capStyle: root.penCapStyle

            PathAngleArc {
				radiusX: internal.baseRadius - internal.radiusOffset
				radiusY: internal.baseRadius - internal.radiusOffset
				centerX: root.width / 2
				centerY: root.height / 2
				startAngle: root.startAngle - 90
				sweepAngle: (root.stopAngle / root.maxValue * root.value)
            }
        }
    }
}
