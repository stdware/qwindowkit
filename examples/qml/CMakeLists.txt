project(QWKExample_QML)

file(GLOB _src *.h *.cpp)

qwk_add_example(${PROJECT_NAME}
    SOURCES ${_src} ../shared/resources/shared.qrc
    QT_LINKS Core Gui Qml Quick
    LINKS QWKQuick
    QML_FILES *.qml
)
