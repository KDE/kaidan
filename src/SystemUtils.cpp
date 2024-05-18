// SPDX-FileCopyrightText: 2024 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "SystemUtils.h"
// Qt
#include <QString>
#include <QSysInfo>

namespace SystemUtils {

QString productName()
{
	const auto productName = QSysInfo::prettyProductName();
	if (productName.contains(u' ')) {
		return productName.section(u' ', 0, -2);
	}
	return productName;
}

}
