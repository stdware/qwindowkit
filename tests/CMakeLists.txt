add_subdirectory(unit)
add_subdirectory(buildsystems)

if(WIN32 AND QWINDOWKIT_BUILD_QUICK)
    add_subdirectory(quickgeometry)
endif()

if(WIN32 AND NOT QWINDOWKIT_FORCE_QT_WINDOW_CONTEXT
    AND (QWINDOWKIT_BUILD_WIDGETS OR QWINDOWKIT_BUILD_QUICK))
    add_subdirectory(windows/hittest)
endif()

if(WIN32 AND QWINDOWKIT_BUILD_QUICK AND QWINDOWKIT_INSTALL)
    add_subdirectory(qmltypes)
endif()

if(WIN32 AND QWINDOWKIT_BUILD_QUICK AND QWINDOWKIT_ENABLE_WINDOWS_SYSTEM_BORDERS)
    add_subdirectory(quickborder)
endif()
