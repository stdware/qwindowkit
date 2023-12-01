project(QWKQuick
    VERSION ${QWINDOWKIT_VERSION}
    LANGUAGES CXX
)

set(_src
    qwkquickglobal.h
    quickitemdelegate_p.h
    quickitemdelegate.cpp
    quickwindowagent.h
    quickwindowagent_p.h
    quickwindowagent.cpp
)

qwk_add_library(${PROJECT_NAME} AUTOGEN
    SOURCES ${_src}
    LINKS QWKCore
    QT_LINKS Core Quick
    QT_INCLUDE_PRIVATE Core Quick
    PREFIX QWK_QUICK
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED TRUE
)