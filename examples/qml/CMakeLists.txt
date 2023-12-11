project(QWKExample_QML)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

file(GLOB _src *.h *.cpp *.qrc)

add_executable(${PROJECT_NAME})

qm_configure_target(${PROJECT_NAME}
    SOURCES ${_src}
    QT_LINKS Core Gui Qml Quick
    LINKS QWKQuick
)

qm_add_win_manifest(${PROJECT_NAME})