add_subdirectory(buildsystems)

if(WIN32 AND QWINDOWKIT_BUILD_QUICK AND QWINDOWKIT_ENABLE_WINDOWS_SYSTEM_BORDERS)
    add_subdirectory(quickborder)
endif()
