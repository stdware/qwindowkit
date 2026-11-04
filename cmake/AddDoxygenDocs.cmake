get_filename_component(_QWK_DOXYGEN_DIR "${CMAKE_CURRENT_LIST_DIR}/../docs/doxygen" ABSOLUTE)
set(_QWK_DOXYGEN_COMMANDS_DIR "${CMAKE_CURRENT_LIST_DIR}/doxygen-commands")

#[[
# Adds a target that generates the HTML documentation with Doxygen, and optionally an installation
# rule that generates it again into the install tree.
#
# qwk_add_doxygen_docs(<target>
#     NAME <name>
#     OUTPUT_DIR <dir>
#     INPUT <paths...>
#     [VERSION <version>]
#     [DESCRIPTION <description>]
#     [MAINPAGE <file>]
#     [TAGFILE <file>]
#     [INSTALL_DIR <dir>]
#     [TARGETS <targets...>]
#     [PREDEFINED <definitions...>]
# )
#
# The settings are read from docs/doxygen/Doxyfile, and the HTML header and the stylesheet of the
# project from the same directory. The theme, doxygen-awesome-css, is downloaded into the current
# binary directory by cmake/doxygen-commands/FetchTheme.cmake each time the documentation is
# generated, which downloads only the files that are absent or have another checksum. Generating
# the documentation for the first time therefore requires network access, and configuring does not.
# After Doxygen, cmake/doxygen-commands/NavTree.cmake corrects the treeview labels.
#
# Doxygen runs in the current source directory, against which relative paths in INPUT and MAINPAGE
# are resolved. MAINPAGE is the Markdown file used as the main page, and is added to the input.
#
# The HTML is written to OUTPUT_DIR/html. TAGFILE is the tag file to write for other projects to
# link against. VERSION defaults to PROJECT_VERSION and DESCRIPTION to NAME.
#
# Doxygen preprocesses the sources with the include directories and the compile definitions of
# TARGETS, followed by PREDEFINED, whose elements have the form NAME or NAME=VALUE. TARGETS are
# built before the documentation.
#
# The target is not part of ALL, because Doxygen is slow and the documentation is requested by
# name. INSTALL_DIR is relative to the install prefix. The installation rule runs Doxygen again
# with INSTALL_DIR as the output directory, instead of copying OUTPUT_DIR, so that installing does
# not require the target to have been built. The installed tag file is named as TAGFILE.
#
# Requires DOXYGEN_EXECUTABLE, which find_package(Doxygen) sets.
#]]
function(qwk_add_doxygen_docs _target)
    set(options)
    set(oneValueArgs NAME VERSION DESCRIPTION MAINPAGE OUTPUT_DIR TAGFILE INSTALL_DIR)
    set(multiValueArgs INPUT TARGETS PREDEFINED)
    cmake_parse_arguments(FUNC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(FUNC_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "qwk_add_doxygen_docs: unknown arguments: ${FUNC_UNPARSED_ARGUMENTS}")
    endif()

    foreach(_arg NAME OUTPUT_DIR INPUT)
        if(NOT FUNC_${_arg})
            message(FATAL_ERROR "qwk_add_doxygen_docs: ${_arg} is required")
        endif()
    endforeach()

    if(NOT DOXYGEN_EXECUTABLE)
        message(FATAL_ERROR "qwk_add_doxygen_docs: DOXYGEN_EXECUTABLE is not set. "
            "Call find_package(Doxygen) first.")
    endif()

    if(FUNC_VERSION)
        set(_version "${FUNC_VERSION}")
    else()
        set(_version "${PROJECT_VERSION}")
    endif()

    if(FUNC_DESCRIPTION)
        set(_description "${FUNC_DESCRIPTION}")
    else()
        set(_description "${FUNC_NAME}")
    endif()

    get_filename_component(_output_dir "${FUNC_OUTPUT_DIR}" ABSOLUTE)

    set(_awesome_dir "${CMAKE_CURRENT_BINARY_DIR}/doxygen-awesome-css")
    set(_fetch_theme
        "${CMAKE_COMMAND}" "-DDIR=${_awesome_dir}"
        -P "${_QWK_DOXYGEN_COMMANDS_DIR}/FetchTheme.cmake"
    )

    # Continues a quoted list on the next line. A value is quoted so that a path may contain
    # spaces.
    set(_sep "\" \\\n    \"")

    set(_input ${FUNC_INPUT})

    if(FUNC_MAINPAGE)
        list(APPEND _input "${FUNC_MAINPAGE}")
    endif()

    list(JOIN _input "${_sep}" _input)

    set(_include_dirs)
    set(_definitions)

    foreach(_item IN LISTS FUNC_TARGETS)
        list(APPEND _include_dirs "$<TARGET_PROPERTY:${_item},INCLUDE_DIRECTORIES>")
        list(APPEND _definitions "$<TARGET_PROPERTY:${_item},COMPILE_DEFINITIONS>")
    endforeach()

    # Empty elements are removed, because an empty quoted value would be an empty include path.
    set(_include_dirs "$<FILTER:${_include_dirs},EXCLUDE,^$>")
    set(_definitions "$<FILTER:${_definitions},EXCLUDE,^$>")

    # Written outside the generator expressions, which a comma or a ">" in a definition would end.
    list(JOIN FUNC_PREDEFINED "${_sep}" _predefined)

    set(_content "@INCLUDE               = \"${_QWK_DOXYGEN_DIR}/Doxyfile\"

PROJECT_NAME           = \"${FUNC_NAME}\"
PROJECT_NUMBER         = \"${_version}\"
PROJECT_BRIEF          = \"${_description}\"
OUTPUT_DIRECTORY       = \"${_output_dir}\"

INPUT                  = \"${_input}\"
")

    if(FUNC_MAINPAGE)
        string(APPEND _content "USE_MDFILE_AS_MAINPAGE = \"${FUNC_MAINPAGE}\"\n")
    endif()

    string(APPEND _content "
INCLUDE_PATH           = $<$<BOOL:${_include_dirs}>:\"$<JOIN:${_include_dirs},${_sep}>\">
PREDEFINED             = $<$<BOOL:${_definitions}>:\"$<JOIN:${_definitions},${_sep}>\">
")

    if(FUNC_PREDEFINED)
        string(APPEND _content "PREDEFINED            += \"${_predefined}\"\n")
    endif()

    string(APPEND _content "
HTML_HEADER            = \"${_QWK_DOXYGEN_DIR}/header.html\"
HTML_EXTRA_STYLESHEET  = \"${_awesome_dir}/doxygen-awesome.css\" \\
    \"${_awesome_dir}/doxygen-awesome-sidebar-only-darkmode-toggle.css\" \\
    \"${_QWK_DOXYGEN_DIR}/extra-stylesheet.css\"
HTML_EXTRA_FILES       = \"${_awesome_dir}/doxygen-awesome-darkmode-toggle.js\"
")

    set(_tagfile_command)

    if(FUNC_TAGFILE)
        get_filename_component(_tagfile "${FUNC_TAGFILE}" ABSOLUTE)
        get_filename_component(_tagfile_dir "${_tagfile}" DIRECTORY)
        get_filename_component(_tagfile_name "${_tagfile}" NAME)
        string(APPEND _content "GENERATE_TAGFILE       = \"${_tagfile}\"\n")
        set(_tagfile_command COMMAND ${CMAKE_COMMAND} -E make_directory "${_tagfile_dir}")
    endif()

    # One file per configuration, because the target properties may depend on it.
    set(_doxyfile "${CMAKE_CURRENT_BINARY_DIR}/${_target}_$<CONFIG>.doxyfile")
    file(GENERATE OUTPUT "${_doxyfile}" CONTENT "${_content}")

    set(_depends)

    if(FUNC_TARGETS)
        set(_depends DEPENDS ${FUNC_TARGETS})
    endif()

    add_custom_target(${_target}
        COMMAND ${_fetch_theme}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_output_dir}"
        ${_tagfile_command}
        COMMAND "${DOXYGEN_EXECUTABLE}" "${_doxyfile}"
        COMMAND ${CMAKE_COMMAND} "-DNAVTREE=${_output_dir}/html/navtree.js"
            -P "${_QWK_DOXYGEN_COMMANDS_DIR}/NavTree.cmake"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Generating the HTML documentation into ${_output_dir}/html"
        VERBATIM
        ${_depends}
    )

    if(NOT FUNC_INSTALL_DIR)
        return()
    endif()

    # The install prefix is known only when installing. The generated Doxyfile is therefore
    # included by a second one that overrides the output directory and the tag file, because a
    # later assignment of a Doxygen setting replaces an earlier one.
    set(_install_doxyfile "${CMAKE_CURRENT_BINARY_DIR}/${_target}_$<CONFIG>_install.doxyfile")
    set(_install_tagfile)

    if(FUNC_TAGFILE)
        set(_install_tagfile
            "string(APPEND _content \"GENERATE_TAGFILE = \\\"\${_dir}/${_tagfile_name}\\\"\\n\")"
        )
    endif()

    install(CODE "
        message(STATUS \"Installing the HTML documentation\")

        # A relative prefix given to cmake --install is relative to the current directory, and
        # Doxygen runs in the source directory.
        get_filename_component(_prefix \"\${CMAKE_INSTALL_PREFIX}\" ABSOLUTE)
        get_filename_component(_dir [==[${FUNC_INSTALL_DIR}]==] ABSOLUTE BASE_DIR \"\${_prefix}\")
        file(MAKE_DIRECTORY \"\${_dir}\")

        set(_content \"@INCLUDE = \\\"${_doxyfile}\\\"\\nOUTPUT_DIRECTORY = \\\"\${_dir}\\\"\\n\")
        ${_install_tagfile}
        file(WRITE [==[${_install_doxyfile}]==] \"\${_content}\")

        execute_process(
            COMMAND [==[${CMAKE_COMMAND}]==] [==[-DDIR=${_awesome_dir}]==]
                -P [==[${_QWK_DOXYGEN_COMMANDS_DIR}/FetchTheme.cmake]==]
            COMMAND_ERROR_IS_FATAL ANY
        )
        execute_process(
            COMMAND [==[${DOXYGEN_EXECUTABLE}]==] [==[${_install_doxyfile}]==]
            WORKING_DIRECTORY [==[${CMAKE_CURRENT_SOURCE_DIR}]==]
            OUTPUT_QUIET
            COMMAND_ERROR_IS_FATAL ANY
        )
        execute_process(
            COMMAND [==[${CMAKE_COMMAND}]==] \"-DNAVTREE=\${_dir}/html/navtree.js\"
                -P [==[${_QWK_DOXYGEN_COMMANDS_DIR}/NavTree.cmake]==]
            COMMAND_ERROR_IS_FATAL ANY
        )
    ")
endfunction()
