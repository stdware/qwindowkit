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

qwk_add_library(${PROJECT_NAME} AUTOGEN
    SOURCES ${_src}
    LINKS QWKCore
    QT_LINKS Core Gui Quick
    QT_INCLUDE_PRIVATE Core Gui Quick
    INCLUDE_PRIVATE
    PREFIX QWK_QUICK
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED TRUE
)

set(QWINDOWKIT_ENABLED_TARGETS ${QWINDOWKIT_ENABLED_TARGETS} ${PROJECT_NAME} PARENT_SCOPE)
set(QWINDOWKIT_ENABLED_SUBDIRECTORIES ${QWINDOWKIT_ENABLED_SUBDIRECTORIES} quick PARENT_SCOPE)
