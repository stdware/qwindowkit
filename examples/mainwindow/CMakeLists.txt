project(QWKExample_MainWindow)

file(GLOB _src *.h *.cpp)

qwk_add_example(${PROJECT_NAME}
    SOURCES ${_src}
    QT_LINKS Core Gui Widgets
    LINKS QWKWidgets WidgetFrame
)