project(QWKExample_QML)

file(GLOB _src *.h *.cpp *.qrc)

qwk_add_example(${PROJECT_NAME}
    FEATURES cxx_std_17
    SOURCES ${_src} ../shared/resources/shared.qrc
    QT_LINKS Core Gui Qml Quick
    LINKS QWKQuick
)