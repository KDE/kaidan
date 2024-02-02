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

XMPP_PROVIDERS_VERSION=2
XMPP_PROVIDERS_ROOT_URL="https://data.xmpp.net/providers/v${XMPP_PROVIDERS_VERSION}"
KAIDAN_SOURCES=$(dirname "$(readlink -f "${0}")")/..

curl \
	-L \
	"${XMPP_PROVIDERS_ROOT_URL}/providers-B.json" \
	> "${KAIDAN_SOURCES}/data/providers.json"

curl \
	-L \
	"${XMPP_PROVIDERS_ROOT_URL}/providers-Ds.json" \
	> "${KAIDAN_SOURCES}/data/providers-completion.json"
