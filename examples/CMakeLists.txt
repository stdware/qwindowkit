set(QWK_EXAMPLES_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Found here, at directory scope. The Qt targets that src/ finds are visible only under src/, and
# QT_VERSION_MAJOR does not reach this directory either. WidgetFrame is always built and requires
# Qt Widgets even if QWINDOWKIT_BUILD_WIDGETS is off.
set(_qt_modules Core Gui Widgets)

if(QWINDOWKIT_BUILD_QUICK)
    list(APPEND _qt_modules Qml Quick)
endif()

if(NOT QT_VERSION_MAJOR)
    find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
endif()

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS ${_qt_modules} REQUIRED)

# Adds an example executable. The arguments are those of qwk_configure_target. A macro, so that
# the automatic Qt tools are enabled for the rest of the calling directory as well.
macro(qwk_add_example _target)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)

    add_executable(${_target})
    qwk_configure_target(${_target} ${ARGN})
    qwk_add_win_rc(${_target} ICON ${QWK_EXAMPLES_DIR}/shared/resources/app/example.ico)
    qwk_add_win_manifest(${_target})
    qwk_add_mac_bundle(${_target} ICON ${QWK_EXAMPLES_DIR}/shared/resources/app/example.icns)
endmacro()

add_subdirectory(shared)

if(QWINDOWKIT_BUILD_WIDGETS)
    add_subdirectory(mainwindow)
endif()

if(QWINDOWKIT_BUILD_QUICK)
    add_subdirectory(qml)
endif()