#[[
# Adds Windows version information and an optional icon to an executable or a shared library.
#
# qwk_add_win_rc(<target>
#     [NAME <name>]
#     [VERSION <version>]
#     [DESCRIPTION <description>]
#     [COPYRIGHT <copyright>]
#     [ICON <ico>]
# )
#
# NAME defaults to the target name, VERSION to PROJECT_VERSION, DESCRIPTION to
# PROJECT_DESCRIPTION, and COPYRIGHT to NAME. A relative ICON is resolved against the current
# source directory. The function has no effect on other platforms and on other target types,
# because a static library has no binary of its own to carry the resource.
#
# A changed icon compiles the resource again at the next build without a configure.
#
# Requires cmake/WriteIfDifferent.cmake.
#]]
function(qwk_add_win_rc _target)
    if(NOT WIN32)
        return()
    endif()

    if(NOT TARGET ${_target})
        message(FATAL_ERROR "qwk_add_win_rc: '${_target}' is not a target")
    endif()

    set(options)
    set(oneValueArgs NAME VERSION DESCRIPTION COPYRIGHT ICON)
    set(multiValueArgs)
    cmake_parse_arguments(FUNC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(FUNC_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "qwk_add_win_rc: unknown arguments: ${FUNC_UNPARSED_ARGUMENTS}")
    endif()

    get_target_property(_type ${_target} TYPE)

    if(NOT _type MATCHES "^(EXECUTABLE|SHARED_LIBRARY)$")
        return()
    endif()

    if(NOT FUNC_NAME)
        set(FUNC_NAME "${_target}")
    endif()

    if(NOT FUNC_VERSION)
        set(FUNC_VERSION "${PROJECT_VERSION}")
    endif()

    if(NOT FUNC_VERSION)
        set(FUNC_VERSION "0.0.0.0")
    endif()

    if(NOT FUNC_DESCRIPTION)
        set(FUNC_DESCRIPTION "${PROJECT_DESCRIPTION}")
    endif()

    if(NOT FUNC_DESCRIPTION)
        set(FUNC_DESCRIPTION "${FUNC_NAME}")
    endif()

    if(NOT FUNC_COPYRIGHT)
        set(FUNC_COPYRIGHT "${FUNC_NAME}")
    endif()

    string(REPLACE "." ";" _parts "${FUNC_VERSION}")
    list(LENGTH _parts _count)

    if(_count GREATER 4)
        message(FATAL_ERROR "qwk_add_win_rc: VERSION has more than four components")
    endif()

    while(_count LESS 4)
        list(APPEND _parts 0)
        math(EXPR _count "${_count} + 1")
    endwhile()

    list(JOIN _parts "," _numeric_version)

    set(_icon_path)

    if(FUNC_ICON)
        get_filename_component(_icon_path "${FUNC_ICON}" ABSOLUTE)
        set(_icon_line " IDI_ICON1               ICON    DISCARDABLE     \"${_icon_path}\"")
    else()
        set(_icon_line "// IDI_ICON1               ICON    DISCARDABLE     \"\"")
    endif()

    set(_content "#include <windows.h>

#ifndef VS_VERSION_INFO
#define VS_VERSION_INFO 1
#endif

${_icon_line}

VS_VERSION_INFO VERSIONINFO
    FILEVERSION    ${_numeric_version}
    PRODUCTVERSION ${_numeric_version}
{
    BLOCK \"StringFileInfo\"
    {
       // U.S. English - Windows, Multilingual
       BLOCK \"040904E4\"
       {
          VALUE \"FileDescription\", \"${FUNC_DESCRIPTION}\"
          VALUE \"FileVersion\", \"${FUNC_VERSION}\"
          VALUE \"ProductName\", \"${FUNC_NAME}\"
          VALUE \"ProductVersion\", \"${FUNC_VERSION}\"
          VALUE \"LegalCopyright\", \"${FUNC_COPYRIGHT}\"
        }
    }
    BLOCK \"VarFileInfo\"
    {
        VALUE \"Translation\", 0x409, 1252 // 1252 = 0x04E4
    }
}
")

    # No generator records the icon as a dependency of the script, and Visual Studio ignores
    # OBJECT_DEPENDS. The script that is compiled is therefore copied at build time by a command
    # that depends on the icon.
    set(_rc "${CMAKE_CURRENT_BINARY_DIR}/${_target}_res.rc")
    qwk_write_if_different("${_rc}.configured" "${_content}")

    add_custom_command(OUTPUT "${_rc}"
        COMMAND ${CMAKE_COMMAND} -E copy "${_rc}.configured" "${_rc}"
        COMMAND ${CMAKE_COMMAND} -E touch "${_rc}"
        DEPENDS "${_rc}.configured" ${_icon_path}
        VERBATIM
    )
    target_sources(${_target} PRIVATE "${_rc}")
endfunction()
