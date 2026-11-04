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
# LINKS, FEATURES and QT_LINKS are public, and LINKS_PRIVATE, INCLUDE_PRIVATE and
# QT_INCLUDE_PRIVATE are private. INCLUDE_PRIVATE takes directories relative to the current source
# directory. QT_LINKS and QT_INCLUDE_PRIVATE take Qt module names without the Qt<major>:: prefix.
#
# QT_INCLUDE_PRIVATE links Qt<major>::<module>Private if that target exists. Otherwise it adds the
# directories of Qt<major><module>_PRIVATE_INCLUDE_DIRS, which only Qt 5 sets, and failing that the
# private header directories under the QT_INSTALL_HEADERS reported by qmake.
#
# Requirements:
#   - Every Qt module that is named, and its Private counterpart, has been looked up with
#     find_package at directory scope. Qt 5 sets the <module>_PRIVATE_INCLUDE_DIRS variables only
#     in the scope of the find_package call that creates its targets, which therefore must not be
#     inside a function.
#   - The project variables QWINDOWKIT_BUILD_STATIC, QWINDOWKIT_BUILD_RUNTIME_DIR,
#     QWINDOWKIT_BUILD_LIBRARY_DIR, QWINDOWKIT_BUILD_INCLUDE_DIR, QWINDOWKIT_INSTALL,
#     QWINDOWKIT_INSTALL_NAME, QWINDOWKIT_INSTALL_NAMESPACE, QWINDOWKIT_DESCRIPTION and
#     QWINDOWKIT_COPYRIGHT are set. If QWINDOWKIT_INSTALL is on, QWINDOWKIT_EXPORT,
#     QWINDOWKIT_INSTALL_RUNTIME_DIR, QWINDOWKIT_INSTALL_LIBRARY_DIR and
#     QWINDOWKIT_INSTALL_INCLUDE_DIR are set as well.
#   - cmake/AddWinRC.cmake is included.
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

    target_sources(${_target} PRIVATE ${FUNC_SOURCES})
    target_link_libraries(${_target} PUBLIC ${FUNC_LINKS})
    target_link_libraries(${_target} PRIVATE ${FUNC_LINKS_PRIVATE})

    set(_include_dirs)

    foreach(_dir IN LISTS FUNC_INCLUDE_PRIVATE)
        get_filename_component(_dir "${_dir}" ABSOLUTE)
        list(APPEND _include_dirs "${_dir}")
    endforeach()

    target_include_directories(${_target} PRIVATE ${_include_dirs})

    target_compile_features(${_target} PUBLIC ${FUNC_FEATURES})

    # CMake adds no -std flag if the default mode of the compiler already supports the requested
    # standard. The clang language server then assumes an older standard, so the standard is also
    # set as a property, which always adds the flag.
    foreach(_feature IN LISTS FUNC_FEATURES)
        if(_feature MATCHES "^cxx_std_(.+)$")
            set_property(TARGET ${_target} PROPERTY CXX_STANDARD ${CMAKE_MATCH_1})
        endif()
    endforeach()

    foreach(_module IN LISTS FUNC_QT_LINKS)
        if(NOT TARGET Qt${QT_VERSION_MAJOR}::${_module})
            message(FATAL_ERROR "qwk_add_library: Qt${QT_VERSION_MAJOR}::${_module} is not found. "
                "Every Qt module must be looked up at directory scope before this call.")
        endif()

        target_link_libraries(${_target} PUBLIC Qt${QT_VERSION_MAJOR}::${_module})
    endforeach()

    foreach(_module IN LISTS FUNC_QT_INCLUDE_PRIVATE)
        _qwk_add_library_include_qt_private(${_target} ${_module})
    endforeach()

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

# Adds the private headers of the Qt module _module to _target.
function(_qwk_add_library_include_qt_private _target _module)
    set(_qt Qt${QT_VERSION_MAJOR})

    if(TARGET ${_qt}::${_module}Private)
        target_link_libraries(${_target} PRIVATE ${_qt}::${_module}Private)
        return()
    endif()

    if(${_qt}${_module}_PRIVATE_INCLUDE_DIRS)
        target_include_directories(${_target} PRIVATE ${${_qt}${_module}_PRIVATE_INCLUDE_DIRS})
        return()
    endif()

    # Qt 6 on Linux has no GuiPrivate target if XKB is not installed, because QtGuiPrivate depends
    # on it. The private headers are then added directly from the header directory of Qt.
    if(NOT QT_INSTALL_HEADERS)
        if(NOT QT_QMAKE_EXECUTABLE)
            if(NOT TARGET ${_qt}::qmake)
                message(FATAL_ERROR "qwk_add_library: the private headers of Qt ${_module} "
                    "require qmake, which is not found")
            endif()

            set(_qmake)

            foreach(_property IMPORTED_LOCATION IMPORTED_LOCATION_RELEASE
                IMPORTED_LOCATION_RELWITHDEBINFO IMPORTED_LOCATION_MINSIZEREL
                IMPORTED_LOCATION_DEBUG)
                get_target_property(_qmake ${_qt}::qmake ${_property})

                if(_qmake)
                    break()
                endif()
            endforeach()

            if(NOT _qmake)
                message(FATAL_ERROR "qwk_add_library: ${_qt}::qmake has no imported location")
            endif()

            set(QT_QMAKE_EXECUTABLE "${_qmake}" CACHE FILEPATH "Path to qmake executable" FORCE)
        endif()

        execute_process(
            COMMAND ${QT_QMAKE_EXECUTABLE} -query QT_INSTALL_HEADERS
            OUTPUT_VARIABLE _install_headers
            OUTPUT_STRIP_TRAILING_WHITESPACE
            COMMAND_ERROR_IS_FATAL ANY
        )

        set(QT_INSTALL_HEADERS ${_install_headers} CACHE PATH "Path to Qt headers" FORCE)
    endif()

    set(_dir ${QT_INSTALL_HEADERS}/Qt${_module}/${QT_VERSION})
    target_include_directories(${_target} PRIVATE
        ${_dir}
        ${_dir}/Qt${_module}
        ${_dir}/Qt${_module}/private
    )
endfunction()
