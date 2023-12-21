project(QWKStyleSupport
    VERSION ${QWINDOWKIT_VERSION}
    LANGUAGES CXX
)

set(_src
    qwkstylesupportglobal.h
    styleagent.h
    styleagent_p.h
    styleagent.cpp
)

set(_links_private)

if(WIN32)
    list(APPEND _src
        styleagent_win.cpp
    )
elseif(APPLE)
    list(APPEND _links_private
        "-framework Foundation"
        "-framework Cocoa"
        "-framework AppKit"
    )
    list(APPEND _src
        styleagent_mac.cpp
    )
else()
    list(APPEND _src
        styleagent_linux.cpp
    )
endif()

qwk_add_library(${PROJECT_NAME} AUTOGEN
    SOURCES ${_src}
    LINKS QWKCore
    LINKS_PRIVATE ${_links_private}
    QT_LINKS Core Gui
    QT_INCLUDE_PRIVATE Core Gui
    PREFIX QWK_STYLESUPPORT
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED TRUE
)

set(QWINDOWKIT_ENABLED_TARGETS ${QWINDOWKIT_ENABLED_TARGETS} ${PROJECT_NAME} PARENT_SCOPE)
set(QWINDOWKIT_ENABLED_SUBDIRECTORIES ${QWINDOWKIT_ENABLED_SUBDIRECTORIES} stylesupport PARENT_SCOPE)