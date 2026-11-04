#[[
# Adds a library target of QWindowKit, together with its export macros, its output directories,
# its installation rule, its Windows version resource and its alias.
#
# qwk_add_library(<target>
#     MACRO_PREFIX <prefix>
#     [QT_AUTOGEN]
#     [SOURCES <files...>]
#     [FEATURES <features...>]
#     [LINKS <libs...>]
#     [LINKS_PRIVATE <libs...>]
#     [INCLUDE_PRIVATE <dirs...>]
#     [QT_LINKS <modules...>]
#     [QT_INCLUDE_PRIVATE <modules...>]
# )
#
# The library is static if QWINDOWKIT_BUILD_STATIC is on, and shared otherwise. <prefix>_LIBRARY is
# defined privately. <prefix>_STATIC is defined publicly for a static library.
#
# SOURCES, FEATURES, LINKS, LINKS_PRIVATE, INCLUDE_PRIVATE, QT_LINKS and QT_INCLUDE_PRIVATE are
# passed to qwk_configure_target, which documents them and their requirements.
#
# Requirements:
#   - The requirements of qwk_configure_target are met.
#   - The project variables QWINDOWKIT_BUILD_STATIC, QWINDOWKIT_BUILD_RUNTIME_DIR,
#     QWINDOWKIT_BUILD_LIBRARY_DIR, QWINDOWKIT_BUILD_INCLUDE_DIR, QWINDOWKIT_INSTALL,
#     QWINDOWKIT_INSTALL_NAME, QWINDOWKIT_INSTALL_NAMESPACE, QWINDOWKIT_DESCRIPTION and
#     QWINDOWKIT_COPYRIGHT are set. If QWINDOWKIT_INSTALL is on, QWINDOWKIT_EXPORT,
#     QWINDOWKIT_INSTALL_RUNTIME_DIR, QWINDOWKIT_INSTALL_LIBRARY_DIR and
#     QWINDOWKIT_INSTALL_INCLUDE_DIR are set as well.
#   - cmake/AddWinRC.cmake and cmake/ConfigureTarget.cmake are included.
#
# A target named QWK<name> is exported as <name> and is aliased as <namespace>::<name>, so that it
# has the same name in the build tree as in the installed package.
#]]
function(qwk_add_library _target)
    set(options QT_AUTOGEN)
    set(oneValueArgs MACRO_PREFIX)
    set(multiValueArgs
        SOURCES FEATURES LINKS LINKS_PRIVATE INCLUDE_PRIVATE QT_LINKS QT_INCLUDE_PRIVATE
    )
    cmake_parse_arguments(FUNC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(FUNC_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "qwk_add_library: unknown arguments: ${FUNC_UNPARSED_ARGUMENTS}")
    endif()

    if(NOT FUNC_MACRO_PREFIX)
        message(FATAL_ERROR "qwk_add_library: MACRO_PREFIX is required")
    endif()

    if(QWINDOWKIT_BUILD_STATIC)
        add_library(${_target} STATIC)
    else()
        add_library(${_target} SHARED)
    endif()

    if(FUNC_QT_AUTOGEN)
        set_target_properties(${_target} PROPERTIES
            AUTOMOC TRUE
            AUTOUIC TRUE
            AUTORCC TRUE
        )
    endif()

    if(QWINDOWKIT_BUILD_STATIC)
        target_compile_definitions(${_target} PUBLIC ${FUNC_MACRO_PREFIX}_STATIC)
    endif()

    target_compile_definitions(${_target} PRIVATE ${FUNC_MACRO_PREFIX}_LIBRARY)

    qwk_configure_target(${_target}
        SOURCES ${FUNC_SOURCES}
        FEATURES ${FUNC_FEATURES}
        LINKS ${FUNC_LINKS}
        LINKS_PRIVATE ${FUNC_LINKS_PRIVATE}
        INCLUDE_PRIVATE ${FUNC_INCLUDE_PRIVATE}
        QT_LINKS ${FUNC_QT_LINKS}
        QT_INCLUDE_PRIVATE ${FUNC_QT_INCLUDE_PRIVATE}
    )

    set_target_properties(${_target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${QWINDOWKIT_BUILD_RUNTIME_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${QWINDOWKIT_BUILD_LIBRARY_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${QWINDOWKIT_BUILD_LIBRARY_DIR}
    )

    if(QWINDOWKIT_INSTALL)
        install(TARGETS ${_target}
            EXPORT ${QWINDOWKIT_EXPORT}
            RUNTIME DESTINATION ${QWINDOWKIT_INSTALL_RUNTIME_DIR}
            LIBRARY DESTINATION ${QWINDOWKIT_INSTALL_LIBRARY_DIR}
            ARCHIVE DESTINATION ${QWINDOWKIT_INSTALL_LIBRARY_DIR}
        )
    endif()

    target_include_directories(${_target} PUBLIC
        $<BUILD_INTERFACE:${QWINDOWKIT_BUILD_INCLUDE_DIR}>
    )

    if(QWINDOWKIT_INSTALL)
        target_include_directories(${_target} PUBLIC
            $<INSTALL_INTERFACE:${QWINDOWKIT_INSTALL_INCLUDE_DIR}>
        )
    endif()

    qwk_add_win_rc(${_target}
        NAME ${QWINDOWKIT_INSTALL_NAME}
        DESCRIPTION "${QWINDOWKIT_DESCRIPTION}"
        COPYRIGHT "${QWINDOWKIT_COPYRIGHT}"
    )

    if(_target MATCHES "^QWK(.+)$")
        set(_name ${CMAKE_MATCH_1})
        set_target_properties(${_target} PROPERTIES EXPORT_NAME ${_name})
    else()
        set(_name ${_target})
    endif()

    add_library(${QWINDOWKIT_INSTALL_NAMESPACE}::${_name} ALIAS ${_target})
endfunction()
