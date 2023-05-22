#!/bin/bash -e
# SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
# SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
# SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
#
# SPDX-License-Identifier: GPL-3.0-or-later

#
# This script updates the provider lists used for registration and autocomplete.
#
# It should be run before each new release.
#

KAIDAN_SOURCES=$(dirname "$(readlink -f "${0}")")/..

curl \
	-L \
	"https://data.xmpp.net/providers/v1/providers-B.json" \
	> "${KAIDAN_SOURCES}/data/providers.json"

curl \
	-L \
	"https://data.xmpp.net/providers/v1/providers-Ds.json" \
	> "${KAIDAN_SOURCES}/data/providers-completion.json"
