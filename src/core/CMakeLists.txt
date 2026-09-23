project(QWKCore
    VERSION ${QWINDOWKIT_VERSION}
    LANGUAGES CXX
)

set(_src
    qwkglobal.h
    qwkglobal_p.h
    qwkglobal.cpp
    windowagentbase.h
    windowagentbase_p.h
    windowagentbase.cpp
    windowitemdelegate_p.h
    windowitemdelegate.cpp
    kernel/eventdispatch_p.h
    kernel/nativeeventfilter_p.h
    kernel/nativeeventfilter.cpp
    kernel/sharedeventfilter_p.h
    kernel/sharedeventfilter.cpp
    kernel/winidchangeeventfilter_p.h
    kernel/winidchangeeventfilter.cpp
    shared/systemwindow_p.h
    contexts/abstractwindowcontext_p.h
    contexts/abstractwindowcontext.cpp
    contexts/qtwindowcontext_p.h
    contexts/qtwindowcontext.cpp
)

set(_links_private)
set(_qt_links_private)

if(WIN32)
    list(APPEND _src
        qwindowkit_windows.h
        qwindowkit_windows.cpp
        shared/qwkwindowsextra_p.h
        shared/windows10borderhandler_p.h
    )
elseif(APPLE)
else()
    list(APPEND _src
        qwindowkit_x11.h
        qwindowkit_x11.cpp
        qwindowkit_wayland.h
        qwindowkit_wayland.cpp
    )
endif()

if(WIN32)
    list(APPEND _src
        contexts/win32windowcontext_p.h
        contexts/win32windowcontext.cpp
    )
    list(APPEND _links_private uxtheme)
elseif(APPLE)
    list(APPEND _src
        contexts/cocoawindowcontext_p.h
        contexts/cocoawindowcontext.mm
    )
    list(APPEND _links_private
        "-framework Foundation"
        "-framework Cocoa"
        "-framework AppKit"
    )
else()
    list(APPEND _src
        contexts/x11context_p.h
        contexts/x11context.cpp
        contexts/waylandcontext_p.h
        contexts/waylandcontext.cpp
    )
endif()

set(_sync_include_options)

if(QWINDOWKIT_ENABLE_STYLE_AGENT)
    list(APPEND _src
        style/styleagent.h
        style/styleagent_p.h
        style/styleagentregistry_p.h
        style/styleagent.cpp
    )

    if(WIN32)
        list(APPEND _src style/styleagent_win.cpp)
    elseif(APPLE)
        list(APPEND _src style/styleagent_mac.mm)
    else()
        list(APPEND _src style/styleagent_linux.cpp
            style/portalsettings_p.h style/portalsettings.cpp style/portalsettings_dbus.cpp)
        list(APPEND _qt_links_private DBus)
    endif()
else()
    list(APPEND _sync_include_options EXCLUDE "src/core/style/.+")
endif()

qwk_add_library(${PROJECT_NAME} QT_AUTOGEN
    SOURCES ${_src}
    FEATURES cxx_std_17
    LINKS_PRIVATE ${_links_private}
    QT_LINKS Core Gui
    QT_LINKS_PRIVATE ${_qt_links_private}
    QT_INCLUDE_PRIVATE Core Gui
    INCLUDE_PRIVATE *
    MACRO_PREFIX QWK_CORE
)

qwk_sync_include(${PROJECT_NAME} FORCE ${_sync_include_options})

# QWKQuick's type registrar needs the inherited methods and SystemButton enum.
if(QWINDOWKIT_BUILD_QUICK AND QT_VERSION VERSION_GREATER_EQUAL 6.3)
    qt6_extract_metatypes(${PROJECT_NAME})
endif()

set(QWINDOWKIT_ENABLED_TARGETS ${QWINDOWKIT_ENABLED_TARGETS} ${PROJECT_NAME} PARENT_SCOPE)
set(QWINDOWKIT_ENABLED_SUBDIRECTORIES ${QWINDOWKIT_ENABLED_SUBDIRECTORIES} core PARENT_SCOPE)
