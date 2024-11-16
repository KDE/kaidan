// SPDX-FileCopyrightText: 2024 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "SystemUtils.h"

// Qt
#include <QLocale>
#include <QString>
#include <QSysInfo>
// Kaidan
#include <Globals.h>

namespace SystemUtils {

LocaleCodes systemLocaleCodes()
{
	const auto systemLocaleParts = QLocale::system().name().split(QStringLiteral("_"));

	// Use the retrieved codes or default values if there are not two codes.
	// An example for usage of the default values is when the "C" locale is
	// retrieved.
	if (systemLocaleParts.size() >= 2) {
		return {
			systemLocaleParts.first().toUpper(),
			systemLocaleParts.last()
		};
	}
	return {
		DEFAULT_LANGUAGE_CODE.toString(),
		DEFAULT_COUNTRY_CODE.toString(),
	};
}

QString productName()
{
	const auto productName = QSysInfo::prettyProductName();
	if (productName.contains(u' ')) {
		return productName.section(u' ', 0, -2);
	}
	return productName;
}

}
