project(QWKQuick
    VERSION ${QWINDOWKIT_VERSION}
    LANGUAGES CXX
)

set(_src
    qwkquickglobal.h
    qwkquickglobal.cpp
    quickitemdelegate_p.h
    quickitemdelegate.cpp
    quickwindowagent.h
    quickwindowagent_p.h
    quickwindowagent.cpp
    quicksystembuttonarea_p.h
    quicksystembuttonarea.cpp
)

if(WIN32)
    list(APPEND _src quickwindowagent_win.cpp)
elseif(APPLE)
    list(APPEND _src quickwindowagent_mac.cpp)
endif()

qwk_add_library(${PROJECT_NAME} QT_AUTOGEN
    SOURCES ${_src}
    FEATURES cxx_std_17
    LINKS QWKCore
    QT_LINKS Core Gui Quick
    QT_INCLUDE_PRIVATE Core Gui Quick
    INCLUDE_PRIVATE *
    MACRO_PREFIX QWK_QUICK
)

qwk_sync_include(${PROJECT_NAME} FORCE)

# Qt's registrar produces both the runtime registration and the tooling metadata.
# Older Qt releases keep the existing procedural registration path.
if(QT_VERSION VERSION_GREATER_EQUAL 6.3)
    set(QWindowKit_QML_IMPORT_PATH "${CMAKE_BINARY_DIR}/qml" CACHE PATH
        "QWindowKit QML tooling import directory" FORCE)
    set(QWINDOWKIT_HAS_QML_MODULE ON PARENT_SCOPE)
    qt6_add_qml_module(${PROJECT_NAME}
        URI QWindowKit
        VERSION 1.0
        NO_PLUGIN
        OUTPUT_DIRECTORY "${QWindowKit_QML_IMPORT_PATH}/QWindowKit"
        RESOURCE_PREFIX /qt/qml
        DEPENDENCIES QtQuick/2.0
        SOURCES qmltypes_p.h
        OUTPUT_TARGETS _qml_output_targets
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE QWK_QUICK_HAS_QML_MODULE)

    if(QWINDOWKIT_INSTALL)
        # Qml is also the module name in older qmsetup releases.
        if(EXISTS "${QMSETUP_MODULES_DIR}/modules/QtQml.cmake")
            qm_import(QtQml)
        else()
            qm_import(Qml)
        endif()
        qm_install_qml_modules(${PROJECT_NAME} PREFIX qml)
        if(_qml_output_targets)
            install(TARGETS ${_qml_output_targets}
                EXPORT ${QWINDOWKIT_EXPORT}
                RUNTIME DESTINATION ${QWINDOWKIT_INSTALL_RUNTIME_DIR}
                LIBRARY DESTINATION ${QWINDOWKIT_INSTALL_LIBRARY_DIR}
                ARCHIVE DESTINATION ${QWINDOWKIT_INSTALL_LIBRARY_DIR}
                OBJECTS DESTINATION ${QWINDOWKIT_INSTALL_LIBRARY_DIR}
            )
        endif()
    endif()
endif()

set(QWINDOWKIT_ENABLED_TARGETS ${QWINDOWKIT_ENABLED_TARGETS} ${PROJECT_NAME} PARENT_SCOPE)
set(QWINDOWKIT_ENABLED_SUBDIRECTORIES ${QWINDOWKIT_ENABLED_SUBDIRECTORIES} quick PARENT_SCOPE)
