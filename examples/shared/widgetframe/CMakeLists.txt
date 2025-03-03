project(WidgetFrame)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

file(GLOB _src *.h *.cpp)

add_library(${PROJECT_NAME} STATIC)

qm_configure_target(${PROJECT_NAME}
    FEATURES cxx_std_17
    SOURCES ${_src}
    QT_LINKS Core Gui Widgets
)

target_include_directories(${PROJECT_NAME} PUBLIC . ..)