#[[
# Adds the installation rules of the export sets and of the CMake package files.
#
# qwk_add_install_package_rule(
#     [NAME <name>]
#     [VERSION <version>]
#     [COMPATIBILITY <compatibility>]
#     [INSTALL_DIR <dir>]
#     [CONFIG_TEMPLATE <file>]
#     [NAMESPACE <namespace>]
#     [EXPORT <sets...>]
#
#     [WRITE_VERSION_OPTIONS <options...>]
#     [WRITE_CONFIG_OPTIONS <options...>]
# )
#
# NAME defaults to PROJECT_NAME, VERSION to PROJECT_VERSION, COMPATIBILITY to AnyNewerVersion,
# and INSTALL_DIR to <libdir>/cmake/<name>. If CONFIG_TEMPLATE is absent or does not exist, the
# generated <name>Config.cmake includes the export files and nothing else.
#
# Include GNUInstallDirs and CMakePackageConfigHelpers before calling this function.
#]]
function(qwk_add_install_package_rule)
    set(options)
    set(oneValueArgs NAME VERSION COMPATIBILITY INSTALL_DIR CONFIG_TEMPLATE NAMESPACE)
    set(multiValueArgs WRITE_VERSION_OPTIONS WRITE_CONFIG_OPTIONS EXPORT)
    cmake_parse_arguments(FUNC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(FUNC_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "qwk_add_install_package_rule: unknown arguments: ${FUNC_UNPARSED_ARGUMENTS}"
        )
    endif()

    if(DEFINED CMAKE_INSTALL_LIBDIR)
        set(_lib_dir "${CMAKE_INSTALL_LIBDIR}")
    else()
        set(_lib_dir lib)
    endif()

    if(FUNC_NAME)
        set(_name "${FUNC_NAME}")
    elseif(PROJECT_NAME)
        set(_name "${PROJECT_NAME}")
    else()
        set(_name unknown)
    endif()

    if(FUNC_VERSION)
        set(_version "${FUNC_VERSION}")
    elseif(PROJECT_VERSION)
        set(_version "${PROJECT_VERSION}")
    else()
        set(_version 0.0.0.0)
    endif()

    if(FUNC_COMPATIBILITY)
        set(_compatibility "${FUNC_COMPATIBILITY}")
    else()
        set(_compatibility AnyNewerVersion)
    endif()

    if(FUNC_INSTALL_DIR)
        set(_install_dir "${FUNC_INSTALL_DIR}")
    else()
        set(_install_dir "${_lib_dir}/cmake/${_name}")
    endif()

    if(FUNC_CONFIG_TEMPLATE)
        set(_config_template "${FUNC_CONFIG_TEMPLATE}")
    else()
        set(_config_template "${CMAKE_CURRENT_LIST_DIR}/${_name}Config.cmake.in")
    endif()

    set(_namespace)

    if(FUNC_NAMESPACE)
        set(_namespace NAMESPACE "${FUNC_NAMESPACE}")
    endif()

    set(_export_files)

    foreach(_item IN LISTS FUNC_EXPORT)
        install(EXPORT "${_item}"
            FILE "${_item}.cmake"
            DESTINATION "${_install_dir}"
            ${_namespace}
        )
        list(APPEND _export_files "${_item}.cmake")
    endforeach()

    get_filename_component(_config_template "${_config_template}" ABSOLUTE)

    if(NOT EXISTS "${_config_template}")
        set(_config_content "@PACKAGE_INIT@\n\n")

        foreach(_item IN LISTS _export_files)
            string(APPEND _config_content
                "include(\"\${CMAKE_CURRENT_LIST_DIR}/${_item}\")\n"
            )
        endforeach()

        set(_config_template "${CMAKE_CURRENT_BINARY_DIR}/${_name}Config.cmake.in")
        file(WRITE "${_config_template}" "${_config_content}")
    endif()

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${_name}ConfigVersion.cmake"
        VERSION "${_version}"
        COMPATIBILITY "${_compatibility}"
        ${FUNC_WRITE_VERSION_OPTIONS}
    )

    configure_package_config_file(
        "${_config_template}"
        "${CMAKE_CURRENT_BINARY_DIR}/${_name}Config.cmake"
        INSTALL_DESTINATION "${_install_dir}"
        ${FUNC_WRITE_CONFIG_OPTIONS}
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${_name}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${_name}ConfigVersion.cmake"
        DESTINATION "${_install_dir}"
    )
endfunction()
