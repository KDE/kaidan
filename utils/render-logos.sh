#!/bin/bash

KAIDAN_SOURCES=$(dirname "$(readlink -f "${0}")")/..

echo "*****************************************"
echo "Rendering logos"
echo "*****************************************"

rendersvg() {
    inkscape -o $2 -w $3 -h $3 $1 >/dev/null
    optipng -quiet -o7 $2 >/dev/null
    advpng -z4 $2 >/dev/null
}

androidlogo() {
    echo "Rendering $KAIDAN_SOURCES/misc/android/res/mipmap-$1..."
    mkdir -p $KAIDAN_SOURCES/misc/android/res/mipmap-$1
    rendersvg $KAIDAN_SOURCES/data/images/kaidan.svg "$KAIDAN_SOURCES/misc/android/res/mipmap-$1/icon.png" $2
    rendersvg $KAIDAN_SOURCES/data/images/kaidan.svg "$KAIDAN_SOURCES/misc/android/res/mipmap-$1/logo.png" $(( $2 * 4 ))
}

androidlogo ldpi 36
androidlogo mdpi 48
androidlogo hdpi 72
androidlogo xhdpi 96
androidlogo xxhdpi 144
androidlogo xxxhdpi 192

