# SPDX-FileCopyrightText: 2022 Plata Hill <plata.hill@kdemail.net>
# SPDX-License-Identifier: BSD-2-Clause

#[=======================================================================[.rst:
QmlFormat
--------------------

This module provides a functionality to format the qml
code of your repository according to the QML Coding Conventions.

This module provides the following function:

::

  qml_format(<files>)

Using this function will create a qml-format target that will format all
``<files>`` passed to the function according to the QML Coding Conventions.
To format the files you have to invoke the target with ``make qml-format`` or ``ninja qml-format``.
Once the project is formatted, it is recommended to enforce the formatting using a pre-commit hook,
this can be done using :kde-module:`KDEGitCommitHooks`.

Example usage:

.. code-block:: cmake

  include(QmlFormat)
  file(GLOB_RECURSE ALL_QML_FORMAT_SOURCE_FILES *.qml)
  qml_format(${ALL_QML_FORMAT_SOURCE_FILES})

#]=======================================================================]

find_package(Qt6 ${QT_MIN_VERSION} NO_MODULE COMPONENTS QmlTools)

# formatting target
function(QML_FORMAT)
    if (TARGET qml-format)
        message(WARNING "the qml_format function was already called")
        return()
    endif()

    # run qml-format only if available, else signal the user what is missing
    if(TARGET Qt6::qmlformat)
        get_target_property(QML_FORMAT_EXECUTABLE Qt6::qmlformat LOCATION)

        # add target without specific commands first, we add the real calls file-per-file to avoid command line length issues
        add_custom_target(qml-format COMMENT "Formatting qml files in ${CMAKE_CURRENT_SOURCE_DIR} with ${QML_FORMAT_EXECUTABLE}...")

        get_filename_component(_binary_dir ${CMAKE_BINARY_DIR} REALPATH)
        foreach(_file ${ARGV})
            # check if the file is inside the build directory => ignore such files
            get_filename_component(_full_file_path ${_file} REALPATH)
            string(FIND ${_full_file_path} ${_binary_dir} _index)
            if(NOT _index EQUAL 0)
                add_custom_command(TARGET qml-format
                    POST_BUILD
                    COMMAND
                        Qt6::qmlformat
                        -i
                        ${_full_file_path}
                    WORKING_DIRECTORY
                        ${CMAKE_CURRENT_SOURCE_DIR}
                    COMMENT
                        "Formatting ${_full_file_path}..."
                    )
            endif()
        endforeach()
    else()
        add_custom_target(qml-format
            COMMAND
                ${CMAKE_COMMAND} -E echo "Could not set up the qml-format target as the qml-format executable is missing."
            )
    endif()
endfunction()
