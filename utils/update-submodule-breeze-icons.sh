#!/bin/bash -e

# SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
#
# SPDX-License-Identifier: GPL-3.0-or-later

#
# This script updates the Git submodule for Breeze Icons.
#
# It should be run before each new release.
#

KAIDAN_DIRECTORY=$(dirname "$(readlink -f "${0}")")/..
BREEZE_ICONS_DIRECTORY="${KAIDAN_DIRECTORY}/3rdparty/breeze-icons"

git submodule update --init --recursive
cd "${BREEZE_ICONS_DIRECTORY}"
git checkout master
cd "${KAIDAN_DIRECTORY}"
git add "${BREEZE_ICONS_DIRECTORY}"
git commit -m "Update Git submodule breeze-icons"
