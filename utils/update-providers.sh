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
# It prints the modified files.
#

XMPP_PROVIDERS_VERSION=2
XMPP_PROVIDERS_ROOT_URL="https://data.xmpp.net/providers/v${XMPP_PROVIDERS_VERSION}"
KAIDAN_ROOT_DIRECTORY=$(dirname "$(readlink -f "${0}")")/..
TARGET_DIRECTORY="${KAIDAN_ROOT_DIRECTORY}/data"
PROVIDER_FILE_NAMES=(
    "providers-B providers"
    "providers-Ds providers-completion"
)

for provider_file_name in "${PROVIDER_FILE_NAMES[@]}"
do
    source_provider_file_name=$(echo "$provider_file_name" | awk '{print $1}')
    source_provider_file_url="${XMPP_PROVIDERS_ROOT_URL}/${source_provider_file_name}.json"

    target_provider_file_name=$(echo "$provider_file_name" | awk '{print $2}')
    target_provider_file_path="${TARGET_DIRECTORY}/${target_provider_file_name}.json"

    curl -s -L "${source_provider_file_url}" > "${target_provider_file_path}"
    git diff --name-only "${target_provider_file_path}"
done
