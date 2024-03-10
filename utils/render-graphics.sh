#!/bin/bash -e

# SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
#
# SPDX-License-Identifier: GPL-3.0-or-later

KAIDAN_DIRECTORY=$(dirname "$(readlink -f "${0}")")/..
OUTPUT_DIRECTORY="${KAIDAN_DIRECTORY}/misc/rendered-graphics"
OPTIMIZE_PNG_SCRIPT="${KAIDAN_DIRECTORY}/utils/optimize-png.sh"

echo "*****************************************"
echo "Rendering graphics"
echo "*****************************************"

render_group_chat_avatar() {
    render_svg_with_margin data/images/kaidan.svg group-chat-avatar.png 300 300 18
}

render_mastodon_avatar() {
    render_svg_with_margin data/images/kaidan.svg mastodon-avatar.png 400 400 18
}

render_mastodon_header() {
    render_svg data/images/chat-page-background.svg mastodon-header.png 824 824
}

# $1: relative input file path
# $2: output filename
# $3: width
# $4: height
# $5: margin
render_svg_with_margin() {
    input_file_path="${KAIDAN_DIRECTORY}/${1}"
    temporary_file_name=$(basename "${1}")
    temporary_file_path="${OUTPUT_DIRECTORY}/tmp-${temporary_file_name}"
    output_file_path="${OUTPUT_DIRECTORY}/${2}"
    inkscape -o "${temporary_file_path}" -w "${3}" -h "${4}" --export-margin="${5}" "${input_file_path}"
    inkscape -o "${output_file_path}" -w "${3}" -h "${4}" "${temporary_file_path}"
    rm "${temporary_file_path}"
    "${OPTIMIZE_PNG_SCRIPT}" "${output_file_path}"
    echo "Created ${output_file_path}"
}

# $1: relative input file path
# $2: output filename
# $3: width
# $4: height
render_svg() {
    input_file_path="${KAIDAN_DIRECTORY}/${1}"
    output_file_path="${OUTPUT_DIRECTORY}/${2}"
    inkscape -o "${output_file_path}" -w "${3}" -h "${4}" "${input_file_path}"
    "${OPTIMIZE_PNG_SCRIPT}" "${output_file_path}"
    echo "Created ${output_file_path}"
}

mkdir -p "${OUTPUT_DIRECTORY}"
render_group_chat_avatar
render_mastodon_avatar
render_mastodon_header
