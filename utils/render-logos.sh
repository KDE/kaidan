#!/bin/bash
# SPDX-FileCopyrightText: 2019 Ilya Bizyaev <bizyaev@zoho.com>
# SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
# SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
# SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
#
# SPDX-License-Identifier: GPL-3.0-or-later

KAIDAN_SOURCES=$(dirname "$(readlink -f "${0}")")/..
OPTIMIZE_PNG_SCRIPT="${KAIDAN_SOURCES}/utils/optimize-png.sh"

echo "*****************************************"
echo "Rendering logos"
echo "*****************************************"

rendersvg() {
    inkscape -o $2 -w $3 -h $3 $1 >/dev/null
    "${OPTIMIZE_PNG_SCRIPT}" "${2}"
}

androidlogo() {
    echo "Rendering $KAIDAN_SOURCES/misc/android/res/mipmap-$1..."
    mkdir -p $KAIDAN_SOURCES/misc/android/res/mipmap-$1
    rendersvg $KAIDAN_SOURCES/data/images/kaidan.svg "$KAIDAN_SOURCES/misc/android/res/mipmap-$1/icon.png" $2
    rendersvg $KAIDAN_SOURCES/data/images/kaidan.svg "$KAIDAN_SOURCES/misc/android/res/mipmap-$1/logo.png" $(( $2 * 4 ))
}

# App icons for ECM
app_icon() {
    rendersvg $KAIDAN_SOURCES/data/images/kaidan.svg "$KAIDAN_SOURCES/misc/app-icons/$1-kaidan.png" $1
    optipng -o9 "$KAIDAN_SOURCES/misc/app-icons/$1-kaidan.png"
}

mkdir -p $KAIDAN_SOURCES/misc/windows

app_icon 16
app_icon 32
app_icon 48
app_icon 256

androidlogo ldpi 36
androidlogo mdpi 48
androidlogo hdpi 72
androidlogo xhdpi 96
androidlogo xxhdpi 144
androidlogo xxxhdpi 192
