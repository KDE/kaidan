// SPDX-FileCopyrightText: 2024 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QString>

namespace SystemUtils
{

struct LocaleCodes {
    QString languageCode;
    QString countryCode;
};

LocaleCodes systemLocaleCodes();

/**
 * Returns a pretty product name of the running local system.
 *
 * This does not contain a version number as QSysInfo::prettyProductName() does.
 */
QString productName();

QString audioDirectory();
QString imageDirectory();
QString videoDirectory();

}
