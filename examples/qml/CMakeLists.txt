project(QWKExample_QML)

file(GLOB _src *.h *.cpp *.qrc)

qwk_add_example(${PROJECT_NAME}
    SOURCES ${_src} ../shared/resources/shared.qrc
    QT_LINKS Core Gui Qml Quick
    LINKS QWKQuick
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED TRUE
)
