cmake_minimum_required(VERSION 3.21)

project(NotesMultiTitlebarDemo
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.10 REQUIRED COMPONENTS Core Gui Widgets)
#find_package(QWindowKit REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(NotesMultiTitlebarDemo WIN32
    src/main.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/sidebartogglebutton.h
    src/sidebartogglebutton.cpp
    src/captionbuttonstrip.h
    src/captionbuttonstrip.cpp
)

target_link_libraries(NotesMultiTitlebarDemo
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        QWindowKit::Widgets
)

target_compile_definitions(NotesMultiTitlebarDemo
    PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
)
