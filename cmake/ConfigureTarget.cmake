#[[
# Adds sources, link libraries, include directories, compile features and Qt modules to a target.
#
# qwk_configure_target(<target>
#     [SOURCES <files...>]
#     [FEATURES <features...>]
#     [LINKS <libs...>]
#     [LINKS_PRIVATE <libs...>]
#     [INCLUDE_PRIVATE <dirs...>]
#     [QT_LINKS <modules...>]
#     [QT_INCLUDE_PRIVATE <modules...>]
# )
#
# SOURCES, LINKS_PRIVATE, INCLUDE_PRIVATE and QT_INCLUDE_PRIVATE are private, and LINKS, FEATURES
# and QT_LINKS are public. INCLUDE_PRIVATE takes directories relative to the current source
# directory. QT_LINKS and QT_INCLUDE_PRIVATE take Qt module names without the Qt<major>:: prefix.
# A cxx_std_<n> feature also sets CXX_STANDARD to <n>.
#
# QT_INCLUDE_PRIVATE links Qt<major>::<module>Private if that target exists. Otherwise it adds the
# directories of Qt<major><module>_PRIVATE_INCLUDE_DIRS, which only Qt 5 sets, and failing that the
# private header directories under the QT_INSTALL_HEADERS reported by qmake.
#
# Requirements:
#   - Every Qt module that is named, and its Private counterpart, has been looked up with
#     find_package at directory scope, in the directory of the call or in one above it. Qt 5 sets
#     the <module>_PRIVATE_INCLUDE_DIRS variables only in the scope of the find_package call that
#     creates its targets, which therefore must not be inside a function.
#   - cmake/GetImportedLocation.cmake is included.
#]]
function(qwk_configure_target _target)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs
        SOURCES FEATURES LINKS LINKS_PRIVATE INCLUDE_PRIVATE QT_LINKS QT_INCLUDE_PRIVATE
    )
    cmake_parse_arguments(FUNC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(FUNC_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "qwk_configure_target: unknown arguments: ${FUNC_UNPARSED_ARGUMENTS}")
    endif()

    if(NOT TARGET ${_target})
        message(FATAL_ERROR "qwk_configure_target: '${_target}' is not a target")
    endif()

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
            message(FATAL_ERROR "qwk_configure_target: Qt${QT_VERSION_MAJOR}::${_module} is not "
                "found. Every Qt module must be looked up at directory scope before this call.")
        endif()

        target_link_libraries(${_target} PUBLIC Qt${QT_VERSION_MAJOR}::${_module})
    endforeach()

    foreach(_module IN LISTS FUNC_QT_INCLUDE_PRIVATE)
        _qwk_configure_target_include_qt_private(${_target} ${_module})
    endforeach()
endfunction()

# Adds the private headers of the Qt module _module to _target.
function(_qwk_configure_target_include_qt_private _target _module)
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
                message(FATAL_ERROR "qwk_configure_target: the private headers of Qt ${_module} "
                    "require qmake, which is not found")
            endif()

            qwk_get_imported_location(${_qt}::qmake _qmake)
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
