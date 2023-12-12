project(QWKCore
    VERSION ${QWINDOWKIT_VERSION}
    LANGUAGES CXX
)

set(_src
    qwkcoreglobal.h
    qwkglobal.h
    qwkglobal_p.h
    windowagentbase.h
    windowagentbase_p.h
    windowagentbase.cpp
    windowitemdelegate_p.h
    windowitemdelegate.cpp
    kernel/eventobserver_p.h
    kernel/eventobserver.cpp
    kernel/nativeeventfilter_p.h
    kernel/nativeeventfilter.cpp
    contexts/abstractwindowcontext_p.h
    contexts/abstractwindowcontext.cpp
)

if(WIN32)
    list(APPEND _src
        qwindowkit_windows.h
        qwindowkit_windows.cpp
        contexts/win32windowcontext_p.h
        contexts/win32windowcontext.cpp
    )
else()
    list(APPEND _src
        contexts/qtwindowcontext_p.h
        contexts/qtwindowcontext.cpp
    )

    if(APPLE)
    # add files
    else()
        list(APPEND _src
            qwindowkit_linux.h
        )
    endif()
endif()

qwk_add_library(${PROJECT_NAME} AUTOGEN
    SOURCES ${_src}
    LINKS
    QT_LINKS Core Gui
    QT_INCLUDE_PRIVATE Core Gui
    INCLUDE_PRIVATE kernel contexts platforms
    PREFIX QWK_CORE
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED TRUE
)

set(QWINDOWKIT_ENABLED_TARGETS ${QWINDOWKIT_ENABLED_TARGETS} ${PROJECT_NAME} PARENT_SCOPE)
set(QWINDOWKIT_ENABLED_SUBDIRECTORIES ${QWINDOWKIT_ENABLED_SUBDIRECTORIES} core PARENT_SCOPE)