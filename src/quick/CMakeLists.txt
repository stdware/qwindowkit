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

set(QWINDOWKIT_ENABLED_TARGETS ${QWINDOWKIT_ENABLED_TARGETS} ${PROJECT_NAME} PARENT_SCOPE)
set(QWINDOWKIT_ENABLED_SUBDIRECTORIES ${QWINDOWKIT_ENABLED_SUBDIRECTORIES} quick PARENT_SCOPE)
