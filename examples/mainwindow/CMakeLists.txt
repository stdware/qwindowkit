project(QWKExample_MainWindow)

file(GLOB _src *.h *.cpp)

qwk_add_example(${PROJECT_NAME}
    SOURCES ${_src} mainwindow.qrc ../shared/resources/shared.qrc
    QT_LINKS Core Gui Widgets # MultimediaWidgets
    LINKS QWKWidgets WidgetFrame
)