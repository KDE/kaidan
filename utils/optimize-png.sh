#!/bin/bash

# SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
#
# SPDX-License-Identifier: GPL-3.0-or-later

if [ "$1" = "--help" ] || [ "$1" = "" ]
then
  echo "Usage: $0 <image file>"
  echo "Example: $0 image.png"
  exit
fi

png_file="$1"

echo "*****************************************"
echo "Optimizing ${png_file}"
echo "*****************************************"

optipng -o7 -quiet "${png_file}" >/dev/null && \
advpng -z4 "${png_file}" >/dev/null
