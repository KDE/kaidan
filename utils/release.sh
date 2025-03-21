#!/bin/bash -e
# SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
#
# SPDX-License-Identifier: GPL-3.0-or-later

kaidan_root_directory=$(readlink -f "$(dirname "$(readlink -f "${0}")")/..")
temporary_data_directory=$(readlink -f "${kaidan_root_directory}/../kaidan-temp-release-data")
log_file_path="${temporary_data_directory}/log"

if [ "${1}" = "-h" ] || [ "${1}" = "--help" ] || [ "${1}" = "" ]
then
    echo "Releases a new version of Kaidan."
    echo
    echo "Phases for major/minor releases:"
    echo "  1. Create stable branch (for final release) and release branch (for merge request)"
    echo "  1.1 Wait to receive translation commits on the stable branch and finalize changelog"
    echo "  2. Update release branch (changelog and date)"
    echo "  3. Merge release branch into stable branch and stable branch into default branch"
    echo "  3.1 Tag release commit"
    echo "  3.2 Publish release files"
    echo
    echo "Phases for patch releases:"
    echo "  1. Create release branch (for merge request)"
    echo "  1.1 Finalize changelog"
    echo "  2. Update release branch (changelog and date)"
    echo "  3. Merge release branch into stable branch and stable branch into default branch"
    echo "  3.1 Tag release commit"
    echo "  3.2 Publish release files"
    echo
    echo "Requirements:"
    echo "  * Git must be locally configured to be used in ${kaidan_root_directory} and ${temporary_data_directory}"
    echo "  * SSH must be configured for invent.kde.org"
    echo
    echo "Dependencies:"
    echo "  * appstreamcli (Debian: 'sudo apt install appstream')"
    echo
    echo "In case of any error, have a look at the log file ${log_file_path}"
    echo
    echo "Usage: $(basename ${0}) <version> [-f]"
    echo
    echo "Example to prepare release (phases 1 & 2) 0.11.0: $(basename ${0}) 0.11.0"
    echo "Example to finalize release (phase 3) 0.11.0: $(basename ${0}) 0.11.0 -f"
    exit
fi

version="${1}"
finalize=$([ "${2}" = "-f" ] && echo "true" || echo "false")

# Used for output on the terminal because all other output is saved to the log file.
user_output_file_descriptor=3

major_version=$(echo "${version}" | awk -F '.' '{print $1}')
minor_version=$(echo "${version}" | awk -F '.' '{print $2}')
patch_version=$(echo "${version}" | awk -F '.' '{print $3}')
stable_version="${major_version}.${minor_version}"

patch=$([ "${patch_version}" != "0" ] && echo "true" || echo "false")

remote="origin"
default_branch="master"
stable_branch="Kaidan/${stable_version}"

# Branches containing "." are automatically protected and thus not allowed for work branches.
release_branch="work/Kaidan/${major_version}-${minor_version}-${patch_version}"
release_commit_message="Release Kaidan ${version}"
release_tag_name="v${version}"
release_tag_message="Kaidan ${version}"
release_merge_request_title_url_encoded=$(echo "${release_commit_message}" | tr "[:blank:]" "+")
release_merge_request_url="https://invent.kde.org/network/kaidan/-/merge_requests/new?merge_request[source_branch]=${release_branch}&merge_request[target_branch]=${stable_branch}&merge_request[title]=${release_merge_request_title_url_encoded}"

git_archive_url="https://invent.kde.org/network/kaidan/-/archive/v${version}/kaidan-v${version}.tar"
git_compressed_archive_url="${git_archive_url}.gz"

release_files_directory_path="${temporary_data_directory}/release-files"
release_files_repository="git@invent.kde.org:network/kaidan.git"
release_files_repository_name=$(echo "${release_files_repository}" | awk -F '/' '{print $2}' | awk -F '.' '{print $1}')
release_files_name_prefix="${release_files_repository_name}-${version}"
release_files_repository_directory_path="${release_files_directory_path}/${release_files_name_prefix}"
release_files_archive_name="${release_files_name_prefix}.tar"
release_files_compressed_archive_name="${release_files_archive_name}.xz"
release_files_upload_url="ftp://upload.kde.org/incoming/"
release_files_download_url="https://download.kde.org/unstable/${release_files_repository_name}/${version}/"
release_files_ticket_url="https://go.kde.org/systickets"

release_announcement_website="https://invent.kde.org/websites/kaidan-im#release-posts"
release_announcement_mastodon="https://fosstodon.org/@kaidan"

metadata_repository="git@invent.kde.org:sysadmin/repo-metadata.git"
metadata_directory_name=$(echo "${metadata_repository}" | awk -F '/' '{print $2}')
metadata_directory_path="${temporary_data_directory}/${metadata_directory_name}"
metadata_file_path="projects-invent/network/kaidan/i18n.json"
metadata_file_content="{\"trunk_kf6\": \"master\", \"stable_kf6\": \""${stable_branch}"\"}"
metadata_branch="work/kaidan/${major_version}-${minor_version}"
metadata_default_branch="master"
metadata_commit_message="kaidan: Update to version ${stable_version}"
metadata_merge_request_title_url_encoded=$(echo "${metadata_commit_message}" | tr "[:blank:]" "+")
metadata_merge_request_description_url_encoded=$(echo "@teams/localization Please merge this if everything is fine" | tr "[:blank:]" "+")
metadata_merge_request_url="https://invent.kde.org/sysadmin/repo-metadata/-/merge_requests/new?merge_request[source_branch]=${metadata_branch}&merge_request[target_branch]=${metadata_default_branch}&merge_request[title]=${metadata_merge_request_title_url_encoded}&merge_request[description]=${metadata_merge_request_description_url_encoded}"

dev_scripts_repository="https://invent.kde.org/sdk/kde-dev-scripts.git"
dev_scripts_directory_name=$(echo "${dev_scripts_repository}" | awk -F '/' '{print $5}')
dev_scripts_directory_path="${temporary_data_directory}/${dev_scripts_directory_name}"
dev_scripts_moc_includes_path="${dev_scripts_directory_path}/addmocincludes"
dev_scripts_moc_includes_commit_message="Add missing moc includes"

xmpp_providers_update_script_path="${kaidan_root_directory}/utils/update-providers.sh"
xmpp_providers_update_commit_message="Update XMPP providers"

copyright_year_update_script_path="${kaidan_root_directory}/utils/update-copyright-year.py"
copyright_year_update_commit_message="Update copyright year"

date=$(date -u --rfc-3339=date)

changelog_file="NEWS.md"
changelog_version_string_prefix="Version "
changelog_version_string="${changelog_version_string_prefix}${version}"
changelog_date_string_prefix="Released: "
changelog_date_string_line="${changelog_date_string_prefix}${date}"
changelog_date_string="${changelog_date_string_line}\n"
changelog_bugfixes_string="Bugfixes:\n"
changelog_features_string="Features:\n"
changelog_notes_string="Notes:\n"

appstream_file="misc/im.kaidan.kaidan.appdata.xml"

doap_file="misc/doap.xml"
doap_release_key="<release>"
doap_release_entry="\\
    <release>\n\
      <Version>\n\
        <revision>${version}</revision>\n\
        <created>${date}</created>\n\
        <file-release rdf:resource='${git_compressed_archive_url}'/>\n\
      </Version>\n\
    </release>\
"

cmake_file="CMakeLists.txt"
cmake_major_version_variable="VERSION_MAJOR"
cmake_minor_version_variable="VERSION_MINOR"
cmake_patch_version_variable="VERSION_PATCH"
cmake_version_code_variable="VERSION_CODE"

wait_for_user_confirmation() {
    printf "Press ENTER once you are finished!" >&${user_output_file_descriptor}
    read
}

find_line_number() {
    file="${1}"
    searched_string="${2}"

    echo $(awk "/${searched_string}/{print NR}" "${file}" | head -n 1)
}

insert_line_into_file() {
    file="${1}"
    line_number="${2}"
    inserted_line="${3}"

    sed -i "${line_number}i${inserted_line}" "${file}"
}

replace_line_in_file() {
    file="${1}"
    replaced_line_start="${2}"
    replacing_line="${3}"

    sed -i "/$replaced_line_start/c\\$replacing_line" "${file}"
}

update_cmake_version() {
    version_variable="${1}"
    version="${2}"

    replace_line_in_file "${cmake_file}" "set(${version_variable}" "set(${version_variable} ${version})"
}

update_cmake_version_code() {
    version_code=$(grep "${cmake_version_code_variable}" "${cmake_file}" | awk '{print $2}' | awk -F ')' '{print $1}')
    ((version_code++))
    replace_line_in_file "${cmake_file}" "set(${cmake_version_code_variable}" "set(${cmake_version_code_variable} ${version_code})"
}

update_repository_metadata() {
    git clone "${metadata_repository}" "${metadata_directory_path}"
    cd "${metadata_directory_path}"

    git checkout -b "${metadata_branch}"

    echo "${metadata_file_content}" > "${metadata_file_path}"
    git add "${metadata_file_path}"
    git commit -m "${metadata_commit_message}"

    git push origin "${metadata_branch}"
    printf "Create merge request: ${metadata_merge_request_url}\n" >&${user_output_file_descriptor}

    cd -
}

run_dev_scripts() {
    git clone "${dev_scripts_repository}" "${dev_scripts_directory_path}"
    modified_files=$(${dev_scripts_moc_includes_path} | awk -F ':' '{print $2}' | awk '{print $NF}')

    if [[ ${modified_files} ]]
    then
        git add ${modified_files}
        git commit -m "${dev_scripts_moc_includes_commit_message}"
    fi
}

update_providers() {
    modified_files=$(${xmpp_providers_update_script_path})

    if [[ ${modified_files} ]]
    then
        git add ${modified_files}
        git commit -m "${xmpp_providers_update_commit_message}"
    fi
}

update_copyright_year() {
    modified_files=$(${copyright_year_update_script_path})

    if [[ ${modified_files} ]]
    then
        git add ${modified_files}
        git commit -m "${copyright_year_update_commit_message}"
    fi
}

update_changelog() {
    latest_version_string_line_number=$(find_line_number "${changelog_file}" "${changelog_version_string_prefix}")
    changelog_version_string_size=${#changelog_version_string}

    if [ "${patch}" = "true" ]
    then
        insert_line_into_file "${changelog_file}" "${latest_version_string_line_number}" "${changelog_bugfixes_string}"
    else
        insert_line_into_file "${changelog_file}" "${latest_version_string_line_number}" "${changelog_notes_string}"
        insert_line_into_file "${changelog_file}" "${latest_version_string_line_number}" "${changelog_bugfixes_string}"
        insert_line_into_file "${changelog_file}" "${latest_version_string_line_number}" "${changelog_features_string}"
    fi

    insert_line_into_file "${changelog_file}" "${latest_version_string_line_number}" "${changelog_date_string}"
    insert_line_into_file "${changelog_file}" "${latest_version_string_line_number}" $(printf '%*s' $changelog_version_string_size "" | tr ' ' '-')
    insert_line_into_file "${changelog_file}" "${latest_version_string_line_number}" "${changelog_version_string}"

    echo >&${user_output_file_descriptor}
    printf "%s\n" \
        "Steps required for ${changelog_file}:" \
        "1. Mention all important changes and their authors" \
        "2. Remove unneeded sections" \
    >&${user_output_file_descriptor}

    wait_for_user_confirmation
}

update_changelog_date() {
    latest_date_string_line_number=$(find_line_number "${changelog_file}" "${changelog_date_string_prefix}")
    sed -i "${latest_date_string_line_number}s/.*/${changelog_date_string_line}/" "${changelog_file}"
}

update_appstream_metadata() {
    appstreamcli news-to-metainfo "${changelog_file}" "${appstream_file}"
}

update_doap() {
    latest_release_line_number=$(find_line_number "${doap_file}" "${doap_release_key}")
    insert_line_into_file "${doap_file}" "${latest_release_line_number}" "${doap_release_entry}"
}

update_cmake() {
    update_cmake_version "${cmake_major_version_variable}" "${major_version}"
    update_cmake_version "${cmake_minor_version_variable}" "${minor_version}"
    update_cmake_version "${cmake_patch_version_variable}" "${patch_version}"
    update_cmake_version_code
}

push_release_branch() {
    git add -u .
    git commit -m "${release_commit_message}"
    git push -f "${remote}" "${release_branch}"
}

set_up_stable_branch() {
    # Create, push and configure a new stable branch if none exists.
    if [ ! $(git ls-remote --exit-code --heads "${remote}" "refs/heads/${stable_branch}") ]
    then
        git checkout -b "${stable_branch}" "${remote}/${default_branch}"
        git push "${remote}" "${stable_branch}"

        update_repository_metadata
    fi
}

set_up_release_branch() {
    # Create a new release branch based on the stable branch.
    git checkout -b "${release_branch}" "${remote}/${stable_branch}"

    run_dev_scripts
    update_providers
    update_copyright_year
    update_changelog
    update_appstream_metadata
    update_doap
    update_cmake

    push_release_branch
    printf "\nCreate merge request: ${release_merge_request_url}\n" >&${user_output_file_descriptor}
}

update_release_branch() {
    git checkout "${release_branch}"
    git pull
    git rebase "${remote}/${stable_branch}"

    printf "You can update "${changelog_file}" now.\n" >&${user_output_file_descriptor}

    wait_for_user_confirmation

    # Reset the release commit while keeping all other commits created for the release.
    git reset HEAD~1

    git restore "${doap_file}"
    update_doap

    update_changelog_date
    update_appstream_metadata

    push_release_branch
}

merge_release_branch() {
    # Merge the release branch into the stable branch if not already done.
    if [ ! $(git log "${remote}/${stable_branch}" --pretty=oneline --grep="${release_commit_message}") ]
    then
        # Rebase the release branch on the stable branch.
        git checkout "${release_branch}"
        git pull
        git rebase "${remote}/${stable_branch}"

        # Merge the release branch into the stable branch.
        git checkout "${stable_branch}"
        git pull
        git merge "${release_branch}"
        git push "${remote}" "${stable_branch}"
    fi

    # Clean up since the release branch is not needed anymore.
    git push -d "${remote}" "${release_branch}"
    git remote prune "${remote}"
    git branch -D "${release_branch}"
}

merge_stable_branch() {
    # Merge the stable branch into the default branch if not already done.
    if [ ! $(git log "${remote}/${default_branch}" --pretty=oneline --grep="${release_commit_message}") ]
    then
        git checkout "${stable_branch}"
        git pull
        git checkout "${default_branch}"
        git pull
        git merge -s ort -Xours --no-edit "${stable_branch}"
        git push "${remote}" "${default_branch}"
    fi

    # Clean up since the stable branch is locally not needed anymore.
    git branch -D "${stable_branch}"
}

tag_release_commit() {
    release_commit=$(git log "${remote}/${default_branch}" --pretty=oneline --grep="${release_commit_message}" | awk '{print $1}')
    git tag -m "${release_tag_message}" "${release_tag_name}" "${release_commit}"
    git push "${remote}" tag "${release_tag_name}"
}

publish_release_files() {
    mkdir "${release_files_directory_path}"
    cd "${release_files_directory_path}"

    # Download, compress and sign the release files.
    git clone --depth=1 -b "${release_tag_name}" "${release_files_repository}" "${release_files_repository_directory_path}"
    rm -rf "${release_files_repository_directory_path}/.git"
    tar -I "xz -9" -cf "${release_files_compressed_archive_name}" "${release_files_name_prefix}"
    gpg -o "${release_files_compressed_archive_name}.sig" -abs "${release_files_compressed_archive_name}"

    # Upload the release files.
    release_files="${release_files_name_prefix}.*"
    release_files_string=$(echo ${release_files} | tr "[:blank:]" ",")
    curl -T "{${release_files_string}}" "${release_files_upload_url}"

    printf "%s\n" \
        "Steps required to publish release files:" \
        "1. Create admin ticket: ${release_files_ticket_url}" \
        "2. Set title: [Kaidan] Release ${version}" \
        "3. Set description:" \
        "Please publish Kaidan's release files on ${release_files_download_url}" \
    >&${user_output_file_descriptor}

    printf '\nSHA-1:\n```\n' >&${user_output_file_descriptor}
    sha1sum ${release_files} >&${user_output_file_descriptor}
    printf '```\n\nSHA-256:\n```\n' >&${user_output_file_descriptor}
    sha256sum ${release_files} >&${user_output_file_descriptor}
    printf '```\n' >&${user_output_file_descriptor}

    cd -
}

announce_release() {
    echo >&${user_output_file_descriptor}
    printf "%s\n" \
        "Steps required to announce release:" \
        "1. Publish release post: ${release_announcement_website}" \
        "2. Publish release toot: ${release_announcement_mastodon}" \
    >&${user_output_file_descriptor}
}

prepare_release() {
    set_up_stable_branch

    # Update an existing release branch.
    # Otherwise, initialize a new one.
    if git show-ref --exists "refs/heads/${release_branch}"
    then
        update_release_branch
    else
        set_up_release_branch
    fi
}

finalize_release() {
    merge_release_branch
    merge_stable_branch
    tag_release_commit
    publish_release_files
    announce_release
}

clear_working_directory() {
    initial_branch=$(git branch --show-current)
    stashed="false"

    if [[ $(git diff --name-only) ]]
    then
        git stash -u
        stashed="true"
    fi
}

restore_working_directory() {
    cd "${kaidan_root_directory}"

    set +e
    git rebase --abort &> /dev/null
    git merge --abort &> /dev/null
    set -e

    git clean -df
    git restore -SW .
    git checkout "${initial_branch}"

    if [ "${stashed}" = "true" ]
    then
        git stash apply
    fi
}

initialize() {
    # Reset an old temporary directory in case of any previous error.
    rm -fr "${temporary_data_directory}"
    mkdir "${temporary_data_directory}"

    # Save the output (except output redirected to $user_output_file_descriptor) in a log file.
    exec {user_output_file_descriptor}>&1 1>>"${log_file_path}" 2>&1 {BASH_XTRACEFD}>&1

    # Set the log output format.
    export PS4='+ ${BASH_SOURCE}:${LINENO}: ${FUNCNAME[0]:+${FUNCNAME[0]}(): }'

    # Start logging.
    set -x

    cd "${kaidan_root_directory}"
    clear_working_directory
    git fetch "${remote}"
}

deinitialize() {
    if [ "${?}" -eq 0 ]
    then
        # Remove all temporary data
        rm -fr "${temporary_data_directory}"
    else
        printf "Execution failed! See ${log_file_path} before function '${FUNCNAME}'\n" >&${user_output_file_descriptor}
    fi

    restore_working_directory
}
trap deinitialize ERR EXIT

main() {
    if [ "${finalize}" = "true" ]
    then
        finalize_release
    else
        prepare_release
    fi
}

initialize
main
