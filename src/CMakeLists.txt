include("conf.cmake")

qwk_init_buildsystem(QWINDOWKIT)

set(QWINDOWKIT_ENABLED_TARGETS)
set(QWINDOWKIT_ENABLED_SUBDIRECTORIES)

# ----------------------------------
# Configurations
# ----------------------------------
set(QMSETUP_DEFINITION_NUMERICAL on)

qm_add_definition(QWINDOWKIT_FORCE_QT_WINDOW_CONTEXT
    CONDITION QWINDOWKIT_FORCE_QT_WINDOW_CONTEXT
)
qm_add_definition(QWINDOWKIT_ENABLE_STYLE_AGENT
    CONDITION QWINDOWKIT_ENABLE_STYLE_AGENT
)
qm_add_definition(QWINDOWKIT_ENABLE_WINDOWS_SYSTEM_BORDERS
    CONDITION QWINDOWKIT_ENABLE_WINDOWS_SYSTEM_BORDERS
)

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
qwk_install()

qwk_finish_buildsystem()

if(QWINDOWKIT_INSTALL)
    # Install shared files
    include("../share/install.cmake")
endif()
