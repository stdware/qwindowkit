# ----------------------------------
# Project Constants
# ----------------------------------
set(QWINDOWKIT_INSTALL_NAMESPACE ${QWINDOWKIT_INSTALL_NAME})
set(QWINDOWKIT_EXPORT ${QWINDOWKIT_INSTALL_NAME}Targets)

set(QWINDOWKIT_INSTALL_CONFIG_TEMPLATE
    "${CMAKE_CURRENT_LIST_DIR}/${QWINDOWKIT_INSTALL_NAME}Config.cmake.in"
)

# QWINDOWKIT_BUILD_DIR is set by the root, unless a parent project has already set it.
set(QWINDOWKIT_BUILD_RUNTIME_DIR ${QWINDOWKIT_BUILD_DIR}/bin)
set(QWINDOWKIT_BUILD_LIBRARY_DIR ${QWINDOWKIT_BUILD_DIR}/lib)
set(QWINDOWKIT_BUILD_INCLUDE_DIR ${QWINDOWKIT_BUILD_DIR}/include)
set(QWINDOWKIT_BUILD_DATA_DIR ${QWINDOWKIT_BUILD_DIR}/share/${QWINDOWKIT_INSTALL_NAME})
set(QWINDOWKIT_BUILD_DOC_DIR ${QWINDOWKIT_BUILD_DIR}/share/doc/${QWINDOWKIT_INSTALL_NAME})

if(QWINDOWKIT_INSTALL)
    # The headers go under a directory named after the package, so that an include reads
    # `<QWKCore/qwkglobal.h>` with `include/QWindowKit` as the include directory.
    set(QWINDOWKIT_INSTALL_INCLUDE_DIR ${CMAKE_INSTALL_INCLUDEDIR}/${QWINDOWKIT_INSTALL_NAME})

    # `share/install.cmake` fills the generated qmake and MSBuild files in from
    # CMAKE_INSTALL_LIBDIR, so the libraries must be installed there as well. Both build system
    # tests fail under -DCMAKE_INSTALL_LIBDIR=lib64 otherwise.
    set(QWINDOWKIT_INSTALL_RUNTIME_DIR ${CMAKE_INSTALL_BINDIR})
    set(QWINDOWKIT_INSTALL_LIBRARY_DIR ${CMAKE_INSTALL_LIBDIR})
    set(QWINDOWKIT_INSTALL_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/${QWINDOWKIT_INSTALL_NAME})

    # Equal to the default of CMAKE_INSTALL_DOCDIR but independent of CMAKE_INSTALL_DATAROOTDIR.
    # Deriving it from that variable would change the installation layout.
    set(QWINDOWKIT_INSTALL_DOC_DIR share/doc/${QWINDOWKIT_INSTALL_NAME})
endif()

# ----------------------------------
# Find Qt
# ----------------------------------
# Found here, at directory scope. qwk_add_library is a function, where a find_package leaves the
# targets behind but takes its variables with it, and Qt 5 sets `Qt5Gui_PRIVATE_INCLUDE_DIRS` only
# on the call that creates the targets.
set(_qt_modules Core Gui)

if(QWINDOWKIT_BUILD_WIDGETS)
    list(APPEND _qt_modules Widgets)
endif()

if(QWINDOWKIT_BUILD_QUICK)
    list(APPEND _qt_modules Quick)
endif()

# A preset QT_VERSION_MAJOR selects the Qt version.
if(NOT QT_VERSION_MAJOR)
    find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
endif()

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS ${_qt_modules} REQUIRED)

# Qt 5 has no private module packages and provides the private header directories as variables.
# A private module that is not found leaves its headers to the fallbacks of qwk_add_library.
if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    foreach(_module IN LISTS _qt_modules)
        if(NOT TARGET Qt${QT_VERSION_MAJOR}::${_module}Private)
            find_package(Qt${QT_VERSION_MAJOR} COMPONENTS ${_module}Private QUIET)
        endif()
    endforeach()
endif()
