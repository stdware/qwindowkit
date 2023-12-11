project(QWKExample_QML)

file(GLOB _src *.h *.cpp *.qrc)

qwk_add_example(${PROJECT_NAME}
    SOURCES ${_src}
    QT_LINKS Core Gui Qml Quick
    LINKS QWKQuick
)