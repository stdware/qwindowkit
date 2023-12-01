project(QWKCore
    VERSION ${QWINDOWKIT_VERSION}
    LANGUAGES CXX
)

set(_src
    qwkcoreglobal.h
    corewindowagent.h
    corewindowagent_p.h
    corewindowagent.cpp
    windowitemdelegate.h
    handler/abstractwindowcontext_p.h
    handler/abstractwindowcontext.cpp
)

if(WIN32)
    list(APPEND _src
        qwindowkit_windows.h
        handler/win32windowcontext_p.h
        handler/win32windowcontext.cpp
    )
else()
    list(APPEND _src
        handler/qtwindowcontext_p.h
        handler/qtwindowcontext.cpp
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
    PREFIX QWK_CORE
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED TRUE
)