project(QWKExample_QML)

qm_find_qt(Quick)
# Read the package version in this scope even if qm_find_qt found an existing target.
find_package(Qt${QT_VERSION_MAJOR}Quick REQUIRED)
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
