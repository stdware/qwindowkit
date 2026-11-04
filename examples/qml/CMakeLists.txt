project(QWKExample_QML)

# Qt<major>Quick_VERSION is set by the find_package call of examples/CMakeLists.txt, which looks
# up Qt Quick whenever this directory is added.
if("${Qt${QT_VERSION_MAJOR}Quick_VERSION}" VERSION_LESS "6.4")
    message(WARNING "Skipping the QtQuick example: FrameAnimation requires Qt 6.4 or later "
                    "(found Qt ${Qt${QT_VERSION_MAJOR}Quick_VERSION})")
    return()
endif()

file(GLOB _src *.h *.cpp *.qrc)

qwk_add_example(${PROJECT_NAME}
    FEATURES cxx_std_17
    SOURCES ${_src} ../shared/resources/shared.qrc
    QT_LINKS Core Gui Qml Quick
    LINKS QWKQuick
)
