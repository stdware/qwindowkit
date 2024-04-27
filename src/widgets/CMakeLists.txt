project(QWKWidgets
    VERSION ${QWINDOWKIT_VERSION}
    LANGUAGES CXX
)

set(_src
    qwkwidgetsglobal.h
    widgetitemdelegate_p.h
    widgetitemdelegate.cpp
    widgetwindowagent.h
    widgetwindowagent_p.h
    widgetwindowagent.cpp
)

if(WIN32)
    list(APPEND _src widgetwindowagent_win.cpp)
elseif(APPLE)
    list(APPEND _src widgetwindowagent_mac.cpp)
endif()

qwk_add_library(${PROJECT_NAME} AUTOGEN
    SOURCES ${_src}
    FEATURES cxx_std_17
    LINKS QWKCore
    QT_LINKS Core Gui Widgets
    QT_INCLUDE_PRIVATE Core Gui Widgets
    INCLUDE_PRIVATE *
    PREFIX QWK_WIDGETS
)

set(QWINDOWKIT_ENABLED_TARGETS ${QWINDOWKIT_ENABLED_TARGETS} ${PROJECT_NAME} PARENT_SCOPE)
set(QWINDOWKIT_ENABLED_SUBDIRECTORIES ${QWINDOWKIT_ENABLED_SUBDIRECTORIES} widgets PARENT_SCOPE)
