// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QQuickStyle>

class GuiStyle : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString name READ name CONSTANT)
	Q_PROPERTY(bool buttonColoringEnabled READ buttonColoringEnabled CONSTANT)
	Q_PROPERTY(bool isMaterial READ isMaterial CONSTANT)

public:
	explicit GuiStyle(QObject *parent = nullptr) : QObject(parent)
	{
	}

	inline static QString name()
	{
		return QQuickStyle::name();
	}

	inline static bool isMaterial()
	{
		static const bool isMaterial = name().compare("Material", Qt::CaseInsensitive) == 0;
		return isMaterial;
	}

	// Not all styles actually support coloring buttons.
	inline static bool buttonColoringEnabled()
	{
		return name() == "Material" || name() == "org.kde.desktop";
	}
};
