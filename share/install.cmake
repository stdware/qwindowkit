# Staged in the build output directory, under the same subdirectories they are installed into.
set(_build_data_dir ${QWINDOWKIT_BUILD_DATA_DIR})

# Install qmake files
if(TRUE)
    set(_qmake_install_dir "${CMAKE_INSTALL_DATADIR}/QWindowKit/qmake")
    file(RELATIVE_PATH _qmake_install_prefix
        "${CMAKE_INSTALL_PREFIX}/${_qmake_install_dir}"
        "${CMAKE_INSTALL_PREFIX}"
    )
    string(REGEX REPLACE "/$" "" _qmake_install_prefix "${_qmake_install_prefix}")

    set(QMAKE_QWK_INSTALL_PREFIX "\$\$PWD/${_qmake_install_prefix}")
    set(QMAKE_QWK_INSTALL_BINDIR ${CMAKE_INSTALL_BINDIR})
    set(QMAKE_QWK_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR})
    set(QMAKE_QWK_INSTALL_INCDIR ${CMAKE_INSTALL_INCLUDEDIR})

    set(QMAKE_QWK_CORE_NAME_RELEASE QWKCore)
    set(QMAKE_QWK_WIDGETS_NAME_RELEASE QWKWidgets)
    set(QMAKE_QWK_QUICK_NAME_RELEASE QWKQuick)

    set(QMAKE_QWK_CORE_NAME_DEBUG QWKCore${CMAKE_DEBUG_POSTFIX})
    set(QMAKE_QWK_WIDGETS_NAME_DEBUG QWKWidgets${CMAKE_DEBUG_POSTFIX})
    set(QMAKE_QWK_QUICK_NAME_DEBUG QWKQuick${CMAKE_DEBUG_POSTFIX})

    if(QWINDOWKIT_BUILD_STATIC)
        set(QMAKE_QWK_CORE_STATIC_MACRO "DEFINES += QWK_CORE_STATIC")
        set(QMAKE_QWK_WIDGETS_STATIC_MACRO "DEFINES += QWK_WIDGETS_STATIC")
        set(QMAKE_QWK_QUICK_STATIC_MACRO "DEFINES += QWK_QUICK_STATIC")
    endif()

    # Only the modules that were built. A `.pri` naming a library that was never installed sends
    # the consumer's linker after a file that is not there.
    set(_qmake_components)

    foreach(_target IN LISTS QWINDOWKIT_ENABLED_TARGETS)
        list(APPEND _qmake_components "${CMAKE_CURRENT_LIST_DIR}/qmake/${_target}.pri.in")
    endforeach()

    foreach(_item IN LISTS _qmake_components)
        get_filename_component(_name ${_item} NAME_WLE)
        configure_file(${_item} ${_build_data_dir}/qmake/${_name} @ONLY)
    endforeach()

    install(DIRECTORY ${_build_data_dir}/qmake/
        DESTINATION ${_qmake_install_dir}
    )
endif()

# Install msbuild files
if(MSVC)
    macro(to_dos_separator _var)
        string(REPLACE "/" "\\" ${_var} ${${_var}})
    endmacro()

    set(_msbuild_install_dir "${CMAKE_INSTALL_DATADIR}/QWindowKit/msbuild")
    file(RELATIVE_PATH _msbuild_install_prefix
        "${CMAKE_INSTALL_PREFIX}/${_msbuild_install_dir}"
        "${CMAKE_INSTALL_PREFIX}"
    )
    string(REGEX REPLACE "/$" "" _msbuild_install_prefix "${_msbuild_install_prefix}")

    set(MSBUILD_QWK_INSTALL_PREFIX "\$(MSBuildThisFileDirectory)${_msbuild_install_prefix}")
    set(MSBUILD_QWK_INSTALL_BINDIR ${CMAKE_INSTALL_BINDIR})
    set(MSBUILD_QWK_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR})
    set(MSBUILD_QWK_INSTALL_INCDIR ${CMAKE_INSTALL_INCLUDEDIR})

    # As with the qmake files, the modules that were built and no others.
    set(MSBUILD_QWK_LIBRARY_LIST_DEBUG)
    set(MSBUILD_QWK_LIBRARY_LIST_RELEASE)

    foreach(_target IN LISTS QWINDOWKIT_ENABLED_TARGETS)
        list(APPEND MSBUILD_QWK_LIBRARY_LIST_DEBUG ${_target}${CMAKE_DEBUG_POSTFIX}.lib)
        list(APPEND MSBUILD_QWK_LIBRARY_LIST_RELEASE ${_target}.lib)
    endforeach()

    to_dos_separator(MSBUILD_QWK_INSTALL_PREFIX)
    to_dos_separator(MSBUILD_QWK_INSTALL_BINDIR)
    to_dos_separator(MSBUILD_QWK_INSTALL_LIBDIR)
    to_dos_separator(MSBUILD_QWK_INSTALL_INCDIR)

    if(QWINDOWKIT_BUILD_STATIC)
        set(MSBUILD_QWK_STATIC_MACRO 
            "<PreprocessorDefinitions>QWK_CORE_STATIC;QWK_WIDGETS_STATIC;QWK_QUICK_STATIC</PreprocessorDefinitions>"
        )
    endif()

    configure_file(${CMAKE_CURRENT_LIST_DIR}/msbuild/QWindowKit.props.in
        ${_build_data_dir}/msbuild/QWindowKit.props
        @ONLY
    )

    install(DIRECTORY ${_build_data_dir}/msbuild/
        DESTINATION ${_msbuild_install_dir}
    )
endif()