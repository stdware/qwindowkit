#[[
# Builds a macOS executable as an application bundle and fills in its Info.plist.
#
# qwk_add_mac_bundle(<target>
#     [NAME <name>]
#     [VERSION <version>]
#     [DESCRIPTION <description>]
#     [COPYRIGHT <copyright>]
#     [ICON <icns>]
# )
#
# NAME defaults to the target name, VERSION to PROJECT_VERSION and then to 0.0.0.0, DESCRIPTION to
# PROJECT_DESCRIPTION and then to NAME, and COPYRIGHT to NAME. NAME is also the output name, the
# bundle name and the bundle identifier. The short version string consists of the first two
# components of VERSION. ICON is copied into Contents/Resources of the bundle. The function has no
# effect on other platforms and on other target types.
#]]
function(qwk_add_mac_bundle _target)
    if(NOT APPLE)
        return()
    endif()

    if(NOT TARGET ${_target})
        message(FATAL_ERROR "qwk_add_mac_bundle: '${_target}' is not a target")
    endif()

    set(options)
    set(oneValueArgs NAME VERSION DESCRIPTION COPYRIGHT ICON)
    set(multiValueArgs)
    cmake_parse_arguments(FUNC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(FUNC_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "qwk_add_mac_bundle: unknown arguments: ${FUNC_UNPARSED_ARGUMENTS}")
    endif()

    get_target_property(_type ${_target} TYPE)

    if(NOT _type STREQUAL "EXECUTABLE")
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

    # A missing second component is 0.
    string(REPLACE "." ";" _parts "${FUNC_VERSION}")
    list(APPEND _parts 0)
    list(GET _parts 0 _major)
    list(GET _parts 1 _minor)

    set_target_properties(${_target} PROPERTIES
        MACOSX_BUNDLE TRUE
        OUTPUT_NAME "${FUNC_NAME}"
        MACOSX_BUNDLE_BUNDLE_NAME "${FUNC_NAME}"
        MACOSX_BUNDLE_EXECUTABLE_NAME "${FUNC_NAME}"
        MACOSX_BUNDLE_INFO_STRING "${FUNC_DESCRIPTION}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${FUNC_NAME}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${FUNC_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${_major}.${_minor}"
        MACOSX_BUNDLE_COPYRIGHT "${FUNC_COPYRIGHT}"
    )

    if(FUNC_ICON)
        # The icon is a source of the executable, which is the only way to have it copied into the
        # bundle. Info.plist names it without a path.
        set_source_files_properties("${FUNC_ICON}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
        get_filename_component(_icon_name "${FUNC_ICON}" NAME)
        set_target_properties(${_target} PROPERTIES MACOSX_BUNDLE_ICON_FILE "${_icon_name}")
        target_sources(${_target} PRIVATE "${FUNC_ICON}")
    endif()
endfunction()
