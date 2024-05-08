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
    kernel/nativeeventfilter_p.h
    kernel/nativeeventfilter.cpp
    kernel/sharedeventfilter_p.h
    kernel/sharedeventfilter.cpp
    kernel/winidchangeeventfilter_p.h
    kernel/winidchangeeventfilter.cpp
    shared/systemwindow_p.h
    contexts/abstractwindowcontext_p.h
    contexts/abstractwindowcontext.cpp
)

set(_links_private)

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
        qwindowkit_linux.h
    )
endif()

if(QWINDOWKIT_ENABLE_QT_WINDOW_CONTEXT)
    list(APPEND _src
        contexts/qtwindowcontext_p.h
        contexts/qtwindowcontext.cpp
    )
else()
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
            contexts/qtwindowcontext_p.h
            contexts/qtwindowcontext.cpp
        )
    endif()
endif()

set(_sync_include_options)

if(QWINDOWKIT_ENABLE_STYLE_AGENT)
    list(APPEND _src
        style/styleagent.h
        style/styleagent_p.h
        style/styleagent.cpp
    )

    if(WIN32)
        list(APPEND _src style/styleagent_win.cpp)
    elseif(APPLE)
        list(APPEND _src style/styleagent_mac.mm)
    else()
        list(APPEND _src style/styleagent_linux.cpp)
    endif()
else()
    list(APPEND _sync_include_options EXCLUDE "src/core/style/\\.+")
endif()

qwk_add_library(${PROJECT_NAME} AUTOGEN
    SOURCES ${_src}
    FEATURES cxx_std_17
    LINKS_PRIVATE ${_links_private}
    QT_LINKS Core Gui
    QT_INCLUDE_PRIVATE Core Gui
    INCLUDE_PRIVATE *
    PREFIX QWK_CORE
    SYNC_INCLUDE_OPTIONS ${_sync_include_options}
)

set(QWINDOWKIT_ENABLED_TARGETS ${QWINDOWKIT_ENABLED_TARGETS} ${PROJECT_NAME} PARENT_SCOPE)
set(QWINDOWKIT_ENABLED_SUBDIRECTORIES ${QWINDOWKIT_ENABLED_SUBDIRECTORIES} core PARENT_SCOPE)