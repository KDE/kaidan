// SPDX-FileCopyrightText: 2024 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

class QString;

namespace SystemUtils {

	/**
	 * Returns a pretty product name of the running local system.
	 *
	 * This does not contain a version number as QSysInfo::prettyProductName() does.
	 */
	QString productName();

}
