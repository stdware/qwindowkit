set(QWK_EXAMPLES_DIR ${CMAKE_CURRENT_SOURCE_DIR})

macro(qwk_add_example _target)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)

    add_executable(${_target})
    qm_configure_target(${_target} ${ARGN})
    qm_add_win_rc(${_target} ICON ${QWK_EXAMPLES_DIR}/shared/resources/app/example.ico)
    qm_add_win_manifest(${_target})
    qm_add_mac_bundle(${_target} ICON ${QWK_EXAMPLES_DIR}/shared/resources/app/example.icns)
endmacro()

add_subdirectory(shared)

if(QWINDOWKIT_BUILD_WIDGETS)
    add_subdirectory(mainwindow)
endif()

if(QWINDOWKIT_BUILD_QUICK)
    add_subdirectory(qml)
endif()