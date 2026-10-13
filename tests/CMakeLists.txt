add_test(NAME testpolicy.timeouts
    COMMAND ${CMAKE_COMMAND}
        "-DBUILD_DIR=${PROJECT_BINARY_DIR}"
        "-DBUILD_CONFIG=$<CONFIG>"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/CheckTimeouts.cmake")
set_tests_properties(testpolicy.timeouts PROPERTIES TIMEOUT 10 LABELS infrastructure)

add_subdirectory(unit)
add_subdirectory(buildsystems)

if(WIN32 AND QWINDOWKIT_BUILD_WIDGETS AND QWINDOWKIT_ENABLE_WINDOWS_SYSTEM_BORDERS)
    add_subdirectory(widgetborder)
endif()

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
