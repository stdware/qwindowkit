project(WidgetFrame)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

file(GLOB _src *.h *.cpp)

add_library(${PROJECT_NAME} STATIC)

qm_configure_target(${PROJECT_NAME}
    SOURCES ${_src}
    QT_LINKS Core Gui Widgets
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED TRUE
)

target_include_directories(${PROJECT_NAME} PUBLIC . ..)