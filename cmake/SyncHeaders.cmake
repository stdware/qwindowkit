#[[
# Makes the headers of a source tree available under <name>/ in an include directory of the
# build tree, and optionally installs them under the same layout.
#
# qwk_sync_headers(<target>
#     BUILD_DIR <dir>
#     [INSTALL_DIR <dir>]
#     [SOURCE_DIR <dir>]
#     [NAME <name>]
#     [EXCLUDE <regex...>]
# )
#
# Every file under SOURCE_DIR with the extension .h, .hh, .hpp or .hxx is collected, in any letter
# case. The directory structure is flattened. A header whose file name contains "_p." goes to
# <name>/private/, and every other header goes to <name>/. A header whose path relative to
# SOURCE_DIR, with forward slashes, matches one of the EXCLUDE expressions is skipped. Two headers
# that flatten to the same destination are reported as an error.
#
# The build tree receives forwarding headers, each of which includes its source header by a
# relative path. The install tree receives copies of the source headers. An empty INSTALL_DIR
# adds no installation rule.
#
# SOURCE_DIR defaults to the current source directory and NAME to the target name. The headers are
# collected at configure time, so a header added later requires a reconfigure.
#
# A forwarding header is written only if its content changes, so that a reconfigure does not
# cause the translation units that include it to be compiled again. The directory is never
# emptied, because other generated files may share it. A forwarding header that the previous
# configure wrote and the current one does not is removed.
#
# Requires cmake/WriteIfDifferent.cmake.
#]]
function(qwk_sync_headers _target)
    set(options)
    set(oneValueArgs BUILD_DIR INSTALL_DIR SOURCE_DIR NAME)
    set(multiValueArgs EXCLUDE)
    cmake_parse_arguments(FUNC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(FUNC_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "qwk_sync_headers: unknown arguments: ${FUNC_UNPARSED_ARGUMENTS}")
    endif()

    if(NOT FUNC_BUILD_DIR)
        message(FATAL_ERROR "qwk_sync_headers: BUILD_DIR is required")
    endif()

    if(FUNC_SOURCE_DIR)
        get_filename_component(_src_dir "${FUNC_SOURCE_DIR}" ABSOLUTE)
    else()
        set(_src_dir "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    if(FUNC_NAME)
        set(_name "${FUNC_NAME}")
    else()
        set(_name "${_target}")
    endif()

    get_filename_component(_build_dir "${FUNC_BUILD_DIR}/${_name}" ABSOLUTE)

    file(GLOB_RECURSE _files RELATIVE "${_src_dir}" LIST_DIRECTORIES false "${_src_dir}/*")
    list(SORT _files)

    set(_written)
    set(_public)
    set(_private)

    foreach(_rel IN LISTS _files)
        string(TOLOWER "${_rel}" _lower)

        if(NOT _lower MATCHES "\\.(h|hh|hpp|hxx)$")
            continue()
        endif()

        set(_excluded off)

        foreach(_pattern IN LISTS FUNC_EXCLUDE)
            if(_rel MATCHES "${_pattern}")
                set(_excluded on)
                break()
            endif()
        endforeach()

        if(_excluded)
            continue()
        endif()

        get_filename_component(_file_name "${_rel}" NAME)

        if(_file_name MATCHES "_p\\.")
            set(_dest_dir "${_build_dir}/private")
            list(APPEND _private "${_src_dir}/${_rel}")
        else()
            set(_dest_dir "${_build_dir}")
            list(APPEND _public "${_src_dir}/${_rel}")
        endif()

        set(_dest "${_dest_dir}/${_file_name}")

        if(_dest IN_LIST _written)
            message(FATAL_ERROR
                "qwk_sync_headers: more than one header of ${_target} flattens to ${_dest}"
            )
        endif()

        file(RELATIVE_PATH _include_path "${_dest_dir}" "${_src_dir}/${_rel}")
        qwk_write_if_different("${_dest}" "#include \"${_include_path}\"\n")
        list(APPEND _written "${_dest}")
    endforeach()

    # The record lists the forwarding headers of this target alone, so that the removal of stale
    # headers never touches another file in the same directory.
    set(_record "${CMAKE_CURRENT_BINARY_DIR}/${_target}_synced_headers.txt")

    if(EXISTS "${_record}")
        file(STRINGS "${_record}" _previous)

        if(_written)
            list(REMOVE_ITEM _previous ${_written})
        endif()

        if(_previous)
            file(REMOVE ${_previous})
        endif()
    endif()

    list(JOIN _written "\n" _record_content)
    qwk_write_if_different("${_record}" "${_record_content}\n")

    if(FUNC_INSTALL_DIR)
        if(_public)
            install(FILES ${_public} DESTINATION "${FUNC_INSTALL_DIR}/${_name}")
        endif()

        if(_private)
            install(FILES ${_private} DESTINATION "${FUNC_INSTALL_DIR}/${_name}/private")
        endif()
    endif()
endfunction()
