#!/bin/bash -e
# SPDX-FileCopyrightText: 2018 Linus Jahn <lnj@kaidan.im>
# SPDX-FileCopyrightText: 2018 Jonah Brüchert <jbb@kaidan.im>
#
# SPDX-License-Identifier: CC0-1.0

. utils/travis/common.sh

echo "*****************************************"
echo "Building Kaidan"
echo "^^^^^^^^^^^^^^^"
echo_env
echo "*****************************************"
echo

mkdir -p ${BUILD_DIR}/build
cd ${BUILD_DIR}/build

if [[ ${PLATFORM} == "ubuntu-touch" ]]; then
	cd ..
	git submodule update --init --remote --checkout
	clickable clean build-libs build click-build review
	if [ ! -z $OPENSTORE_API_KEY ] && [ ! -z $CI_COMMIT_TAG ]; then
		clickable publish
	fi
	mv bin/ubuntu-touch/*.click .
elif [[ ${BUILD_SYSTEM} == "cmake" ]]; then
	cmake .. \
	      -GNinja \
	      -DCMAKE_BUILD_TYPE=Debug \
	      -DBUILD_TESTS=ON \
	      -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
	      -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER:-"g++"}

	cmake --build .
	CTEST_OUTPUT_ON_FAILURE=1 cmake --build . --target test
else
	echo "Unknown platform or build system!"
	exit 1
fi
