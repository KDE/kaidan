#!/bin/bash
# SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
#
# SPDX-License-Identifier: GPL-3.0-or-later

SCRIPT_DIR=$(dirname "${0}")

if [ "${SCRIPT_DIR:0:1}" != "/" ]; then
    SCRIPT_DIR=${PWD}/${SCRIPT_DIR}
fi

SOURCE_DIR="${SCRIPT_DIR}/../src"

# Force early exit on issue
set -e
# Force debug traces of executed commands
#set -o xtrace

# To see current imports:
# find "${SOURCE_DIR}" -iname '*.qml' -exec grep 'import ' {} \; | sort -u

declare -A QT
QT["QtGraphicalEffects"]="1.14"
QT["QtLocation"]="5.14"
QT["QtMultimedia"]="5.14"
QT["QtPositioning"]="5.14"
QT["QtQuick"]="2.14"
QT["QtQuick.Controls"]="2.14"
QT["QtQuick.Controls.Material"]="2.14"
QT["QtQuick.Dialogs"]="1.3"
QT["QtQuick.Layouts"]="1.14"

declare -A KIRIGAMI
KIRIGAMI["org.kde.kirigami"]="2.19"

declare -A KAIDAN
KAIDAN["EmojiModel"]="0.1"
KAIDAN["MediaUtils"]="0.1"
KAIDAN["StatusBar"]="0.1"
KAIDAN["im.kaidan.kaidan"]="1.0"

SED_REPLACES=

for key in "${!QT[@]}"
do
    SED_REPLACES="${SED_REPLACES} -e 's/[[:space:]]*import[[:space:]]+${key//./\\.}[[:space:]]+[0-9]+\.[0-9]+/import ${key} ${QT[${key}]}/'"
done

for key in "${!KIRIGAMI[@]}"
do
    SED_REPLACES="${SED_REPLACES} -e 's/[[:space:]]*import[[:space:]]+${key//./\\.}[[:space:]]+[0-9]+\.[0-9]+/import ${key} ${KIRIGAMI[${key}]}/'"
done

for key in "${!KAIDAN[@]}"
do
    SED_REPLACES="${SED_REPLACES} -e 's/[[:space:]]*import[[:space:]]+${key//./\\.}[[:space:]]+[0-9]+\.[0-9]+/import ${key} ${KAIDAN[${key}]}/'"
done

SED_CMD="sed -Ei'' ${SED_REPLACES}"

while IFS= read -r qml_file; do
    echo "Updating ${qml_file}..."
    eval ${SED_CMD} "${qml_file}"
done < <( find "${SOURCE_DIR}" -type f -iname '*.qml' )
