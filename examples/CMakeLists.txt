set(QWK_EXAMPLES_DIR ${CMAKE_CURRENT_SOURCE_DIR})

macro(qwk_add_example _target)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)

    add_executable(${_target})
    qm_configure_target(${_target} ${ARGN})
    qm_add_win_rc(${_target} ICON ../shared/resources/example.ico)
    qm_add_win_manifest(${_target})
endmacro()

add_subdirectory(shared)

add_subdirectory(mainwindow)

add_subdirectory(qml)