include("conf.cmake")

set(QWINDOWKIT_ENABLED_TARGETS)
set(QWINDOWKIT_ENABLED_SUBDIRECTORIES)

# Passed to qwk_sync_headers by each module. Empty if nothing is installed, which adds no
# installation rule for the headers.
set(QWINDOWKIT_HEADER_INSTALL_DIR)

if(QWINDOWKIT_INSTALL)
    set(QWINDOWKIT_HEADER_INSTALL_DIR ${QWINDOWKIT_INSTALL_INCLUDE_DIR})
endif()

# ----------------------------------
# Main Project
# ----------------------------------
add_subdirectory(core)

if(QWINDOWKIT_BUILD_WIDGETS)
    add_subdirectory(widgets)
endif()

if(QWINDOWKIT_BUILD_QUICK)
    add_subdirectory(quick)
endif()

# ----------------------------------
# Configurations
# ----------------------------------
# Shares the directory of the forwarding headers of QWKCore, which qwk_sync_headers never empties.
set(_config_header ${QWINDOWKIT_BUILD_INCLUDE_DIR}/QWKCore/qwkconfig.h)

qwk_generate_config_header(${_config_header}
    GUARD QWINDOWKIT_QWKCONFIG_H
    FEATURES
        QWINDOWKIT_FORCE_QT_WINDOW_CONTEXT
        QWINDOWKIT_ENABLE_STYLE_AGENT
        QWINDOWKIT_ENABLE_WINDOWS_SYSTEM_BORDERS
)

# ----------------------------------
# Documentation
# ----------------------------------
if(QWINDOWKIT_BUILD_DOCUMENTATIONS)
    if(NOT DOXYGEN_EXECUTABLE)
        find_package(Doxygen REQUIRED)
    endif()

    set(_install_options)

    if(QWINDOWKIT_INSTALL)
        set(_install_options INSTALL_DIR ${QWINDOWKIT_INSTALL_DOC_DIR})
    endif()

    set(_doc_targets ${QWINDOWKIT_ENABLED_TARGETS})

    set(QWINDOWKIT_DOXYGEN_TAGFILE
        ${QWINDOWKIT_BUILD_DOC_DIR}/${QWINDOWKIT_INSTALL_NAME}_tagfile.xml
        CACHE FILEPATH "QWindowKit doxygen tag file path" FORCE
    )

    qm_import(Doxygen)
    qm_setup_doxygen(QWindowKit_RunDoxygen
        NAME "QWindowKit"
        DESCRIPTION "${QWINDOWKIT_DESCRIPTION}"
        MDFILE ../README.md
        OUTPUT_DIR ${QWINDOWKIT_BUILD_DOC_DIR}
        INPUT ${QWINDOWKIT_ENABLED_SUBDIRECTORIES}
        TARGETS ${_doc_targets}
        DEPENDS ${_doc_targets}
        NO_EXPAND_MACROS Q_OBJECT Q_GADGET Q_DECLARE_TR_FUNCTIONS
        COMPILE_DEFINITIONS Q_SIGNALS=Q_SIGNALS Q_SLOTS=Q_SLOTS
        GENERATE_TAGFILE "${QWINDOWKIT_DOXYGEN_TAGFILE}"
        ${_install_options}
    )
endif()

# ----------------------------------
# Install
# ----------------------------------
if(QWINDOWKIT_INSTALL)
    # qwk_add_library adds the library targets to the export set.
    qwk_add_install_package_rule(
        NAME ${QWINDOWKIT_INSTALL_NAME}
        VERSION ${QWINDOWKIT_VERSION}
        INSTALL_DIR ${QWINDOWKIT_INSTALL_CMAKE_DIR}
        CONFIG_TEMPLATE ${QWINDOWKIT_INSTALL_CONFIG_TEMPLATE}
        NAMESPACE ${QWINDOWKIT_INSTALL_NAMESPACE}::
        EXPORT ${QWINDOWKIT_EXPORT}
        WRITE_CONFIG_OPTIONS NO_CHECK_REQUIRED_COMPONENTS_MACRO
    )

    install(FILES ${_config_header} DESTINATION ${QWINDOWKIT_INSTALL_INCLUDE_DIR}/QWKCore)
endif()

if(QWINDOWKIT_INSTALL)
    # Install shared files
    include("../share/install.cmake")
endif()
