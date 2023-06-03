#!/bin/bash
# SPDX-FileCopyrightText: 2018 Ilya Bizyaev <bizyaev@zoho.com>
# SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
#
# SPDX-License-Identifier: GPL-3.0-or-later

# NOTE: To use this script, you need to set ANDROID_NDK_ROOT, ANDROID_SDK_ROOT
# and QT_ANDROID to your Qt for Android installation

if [ -z "$ANDROID_NDK_ROOT" ]; then
    echo "ANDROID_NDK_ROOT has to be set"
    exit 1
fi

if [ -z "$ANDROID_SDK_ROOT" ]; then
    echo "ANDROID_SDK_ROOT has to be set"
    exit 1
fi

if [ -z "$QT_ANDROID" ]; then
    echo "QT_ANDROID has to be set"
    exit 1
fi

# Build type is one of:
# Debug, Release, RelWithDebInfo and MinSizeRel
BUILD_TYPE="${BUILD_TYPE:-Debug}"
# Android SDK Tools version available on the system
ANDROID_SDK_BUILD_TOOLS_REVISION=${ANDROID_SDK_BUILD_TOOLS_REVISION:-25.0.3}
# Build API version
ANDROID_API_VERSION=21
# Build architecture
ANDROID_ARCH=${ANDROID_ARCH:-armv7}

KAIDAN_SOURCES=$(dirname "$(readlink -f "${0}")")/..
OPENSSL_PATH=/tmp/openssl
OPENSSL_SETENV=$OPENSSL_PATH/Setenv-android.sh
CUSTOM_ANDROID_TOOLCHAIN=/tmp/android-arm-toolchain

echo "-- Starting $BUILD_TYPE build of Kaidan --"

echo "*****************************************"
echo "Fetching dependencies if required"
echo "*****************************************"

mkdir -p $KAIDAN_SOURCES/3rdparty

if [ ! -f "$KAIDAN_SOURCES/3rdparty/kirigami/.git" ]; then
    echo "Cloning Kirigami"
    git clone https://invent.kde.org/frameworks/kirigami $KAIDAN_SOURCES/3rdparty/kirigami
fi

if [ ! -e "$KAIDAN_SOURCES/3rdparty/breeze-icons/.git" ]; then
    echo "Cloning Breeze icons"
    git clone https://invent.kde.org/frameworks/breeze-icons $KAIDAN_SOURCES/3rdparty/breeze-icons
fi

if [ ! -e "$KAIDAN_SOURCES/3rdparty/qxmpp/.git" ]; then
    echo "Cloning QXmpp"
    git clone https://github.com/qxmpp-project/qxmpp.git $KAIDAN_SOURCES/3rdparty/qxmpp
fi

if [ ! -d "$OPENSSL_PATH" ]; then
    echo "Cloning OpenSSL into $OPENSSL_PATH"
    git clone --branch OpenSSL_1_0_2-stable --depth=1 git://git.openssl.org/openssl.git $OPENSSL_PATH
fi

if [ ! -f "$OPENSSL_SETENV" ]; then
    echo "Fetching OpenSSL's Setenv-android.sh"
    wget https://wiki.openssl.org/images/7/70/Setenv-android.sh -O $OPENSSL_SETENV --continue
    dos2unix $OPENSSL_SETENV
    sed -i 's/arm-linux-androideabi-4.8/arm-linux-androideabi-4.9/g' $OPENSSL_SETENV
    sed -i 's/android-18/android-$ANDROID_API_VERSION/g' $OPENSSL_SETENV
    chmod +x $OPENSSL_SETENV
fi

if [ ! -d "$CUSTOM_ANDROID_TOOLCHAIN" ]; then
    echo "*****************************************"
    echo "Preparing Android toolchain"
    echo "*****************************************"

    echo "ARM, API $ANDROID_API_VERSION"
    $ANDROID_NDK_ROOT/build/tools/make_standalone_toolchain.py --arch arm --api $ANDROID_API_VERSION --install-dir $CUSTOM_ANDROID_TOOLCHAIN
fi

if [ ! -f "$CUSTOM_ANDROID_TOOLCHAIN/lib/libssl.so" ]; then
echo "*****************************************"
echo "Building OpenSSL"
echo "*****************************************"

{
    cd $OPENSSL_PATH
    source $OPENSSL_SETENV
    export ANDROID_NDK=$ANDROID_NDK_ROOT

    case ${ANDROID_ARCH} in
        x86)
            sed -ie 's/_ANDROID_ARCH=.*$/_ANDROID_ARCH="arch-x86"/' $OPENSSL_SETENV
            sed -ie 's/_ANDROID_EABI=.*$/_ANDROID_EABI="x86-4.9"/' $OPENSSL_SETENV
        ;;
        armv7)
            sed -ie 's/_ANDROID_ARCH=.*$/_ANDROID_ARCH="arch-arm"/' $OPENSSL_SETENV
            sed -ie 's/_ANDROID_EABI=.*$/_ANDROID_EABI="arm-linux-androideabi-4.9"/' $OPENSSL_SETENV
        ;;
    esac

    ./Configure --prefix=$CUSTOM_ANDROID_TOOLCHAIN shared android-${ANDROID_ARCH}
    #./Configure -I${ANDROID_NDK_ROOT}/sysroot/usr/include -I${ANDROID_NDK_ROOT}/sysroot/usr/include/${CROSS_COMPILE::-1}/ shared android-${ANDROID_ARCH}

    make depend
    make CALC_VERSIONS="SHLIB_COMPAT=; SHLIB_SOVER=" build_libs -j$(nproc) || return
    make -k CALC_VERSIONS="SHLIB_COMPAT=; SHLIB_SOVER=" install_sw -j$(nproc)
    cp -v libcrypto.so libssl.so $CUSTOM_ANDROID_TOOLCHAIN/lib
}
fi

cdnew() {
    if [ -d "$1" ]; then
        rm -rf "$1"
    fi
    mkdir $1
    cd $1
}

if [ ! -f "$CUSTOM_ANDROID_TOOLCHAIN/lib/pkgconfig/qxmpp.pc" ]; then
echo "*****************************************"
echo "Building QXmpp"
echo "*****************************************"
{
    cdnew $KAIDAN_SOURCES/3rdparty/qxmpp/build
    cmake .. \
        -DCMAKE_TOOLCHAIN_FILE=/usr/share/ECM/toolchain/Android.cmake \
        -DECM_ADDITIONAL_FIND_ROOT_PATH=$QT_ANDROID \
        -DANDROID_NDK=$ANDROID_NDK_ROOT \
        -DANDROID_SDK_ROOT=$ANDROID_SDK_ROOT \
        -DANDROID_SDK_BUILD_TOOLS_REVISION=$ANDROID_SDK_BUILD_TOOLS_REVISION \
        -DCMAKE_PREFIX_PATH=$QT_ANDROID \
        -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=OFF \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_INSTALL_PREFIX=$CUSTOM_ANDROID_TOOLCHAIN

    make -j$(nproc)
    make install
    rm -rf $KAIDAN_SOURCES/3rdparty/qxmpp/build
}
fi

if [ ! -f "$CUSTOM_ANDROID_TOOLCHAIN/lib/libKF5Kirigami2.so" ]; then
echo "*****************************************"
echo "Building Kirigami"
echo "*****************************************"
{
    cdnew $KAIDAN_SOURCES/3rdparty/kirigami/build
    cmake .. \
        -DCMAKE_TOOLCHAIN_FILE=/usr/share/ECM/toolchain/Android.cmake \
        -DECM_ADDITIONAL_FIND_ROOT_PATH=$QT_ANDROID \
        -DCMAKE_PREFIX_PATH=$QT_ANDROID/ \
        -DANDROID_NDK=$ANDROID_NDK_ROOT \
        -DANDROID_SDK_ROOT=$ANDROID_SDK_ROOT \
        -DANDROID_SDK_BUILD_TOOLS_REVISION=$ANDROID_SDK_BUILD_TOOLS_REVISION \
        -DCMAKE_INSTALL_PREFIX=$CUSTOM_ANDROID_TOOLCHAIN \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE
    make -j$(nproc)
    make install
    rm -rf $KAIDAN_SOURCES/3rdparty/kirigami/build
}
fi

export PKG_CONFIG_PATH=$CUSTOM_ANDROID_TOOLCHAIN/lib/pkgconfig

echo "*****************************************"
echo "Building Kaidan"
echo "*****************************************"
{
    cdnew $KAIDAN_SOURCES/build
    cmake .. \
        -DQTANDROID_EXPORTED_TARGET=kaidan -DECM_DIR=/usr/share/ECM/cmake \
        -DCMAKE_TOOLCHAIN_FILE=/usr/share/ECM/toolchain/Android.cmake \
        -DECM_ADDITIONAL_FIND_ROOT_PATH="$QT_ANDROID;$CUSTOM_ANDROID_TOOLCHAIN" \
        -DCMAKE_PREFIX_PATH=$QT_ANDROID \
        -DANDROID_NDK=$ANDROID_NDK_ROOT -DCMAKE_ANDROID_NDK=$ANDROID_NDK_ROOT \
        -DANDROID_SDK_ROOT=$ANDROID_SDK_ROOT \
        -DANDROID_SDK_BUILD_TOOLS_REVISION=$ANDROID_SDK_BUILD_TOOLS_REVISION \
        -DCMAKE_INSTALL_PREFIX=$CUSTOM_ANDROID_TOOLCHAIN \
        -DANDROID_APK_DIR=../misc/android -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DI18N=1
    make create-apk-kaidan -j$(nproc)
}

if [ ! -z "$INSTALL" ]; then
    echo "*****************************************"
    echo "Installing to device"
    echo "*****************************************"

    cd $KAIDAN_SOURCES/build
    make install-apk-kaidan
fi
