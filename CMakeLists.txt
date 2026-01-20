cmake_minimum_required(VERSION 3.19)

project(QWindowKit
    VERSION 1.5.1.0
    LANGUAGES CXX
    HOMEPAGE_URL "https://github.com/stdware/qwindowkit"
    DESCRIPTION "Cross-platform window customization framework"
)

# ----------------------------------
# Build Options
# ----------------------------------
option(QWINDOWKIT_BUILD_STATIC "Build static libraries" OFF)
option(QWINDOWKIT_BUILD_WIDGETS "Build widgets module" ON)
option(QWINDOWKIT_BUILD_QUICK "Build quick module" OFF)
option(QWINDOWKIT_BUILD_EXAMPLES "Build examples" OFF)
option(QWINDOWKIT_BUILD_DOCUMENTATIONS "Build documentations" OFF)
option(QWINDOWKIT_INSTALL "Install library" ON)

option(QWINDOWKIT_FORCE_QT_WINDOW_CONTEXT "Force use Qt Window Context" OFF)
option(QWINDOWKIT_ENABLE_WINDOWS_SYSTEM_BORDERS "Enable system borders on Windows" ON)
option(QWINDOWKIT_ENABLE_STYLE_AGENT "Enable building style agent" ON)

#[[

Detailed Introcuction to Configure Options:

`QWINDOWKIT_BUILD_DOCUMENTATIONS`
  - If you have installed `Doxygen`, you can ENABLE this option so that the documentations
    will also be built and installed.
  - If not, you can read the comments in `qdoc` style in `cpp` files to get detailed usages
    of the public APIs.

`QWINDOWKIT_ENABLE_WINDOWS_SYSTEM_BORDERS`
  - If you don't want the system borders on Windows 10/11, you can DISABLE this option.
  - If so, the Windows 10 top border issue will disappear. However, part of the client edge
    area will be occupied as the resizing margins.

`QWINDOWKIT_FORCE_QT_WINDOW_CONTEXT`
  - If you want to use pure Qt emulated frameless implementation, you can ENABLE this option.
  - If so, all system native features will be lost.
  - The Qt Window Context is supported on all platforms.

`QWINDOWKIT_ENABLE_STYLE_AGENT`
  - Select whether to exclude the style component by DISABLING this option according to your
    requirements and your Qt version.

#]]

# ----------------------------------
# CMake Settings
# ----------------------------------
if(MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /manifest:no")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /manifest:no")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /manifest:no")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")

    if(NOT DEFINED CMAKE_DEBUG_POSTFIX)
        set(CMAKE_DEBUG_POSTFIX "d")
    endif()
elseif(MINGW)
    if(NOT DEFINED CMAKE_STATIC_LIBRARY_PREFIX)
        set(CMAKE_STATIC_LIBRARY_PREFIX "")
    endif()

    if(NOT DEFINED CMAKE_SHARED_LIBRARY_PREFIX)
        set(CMAKE_SHARED_LIBRARY_PREFIX "")
    endif()

    if(NOT DEFINED CMAKE_IMPORT_LIBRARY_PREFIX)
        set(CMAKE_IMPORT_LIBRARY_PREFIX "")
    endif()
endif()

if(NOT DEFINED CMAKE_BUILD_TYPE AND NOT DEFINED CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release")
endif()

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
    set(CMAKE_INSTALL_RPATH "\$ORIGIN:\$ORIGIN/../lib")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH OFF)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set(CMAKE_C_VISIBILITY_PRESET "hidden")
set(CMAKE_CXX_VISIBILITY_PRESET "hidden")

if(QWINDOWKIT_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)
endif()

# ----------------------------------
# Project Variables
# ----------------------------------
set(QWINDOWKIT_VERSION ${PROJECT_VERSION})
set(QWINDOWKIT_INSTALL_NAME ${PROJECT_NAME})

string(TIMESTAMP _QWINDOWKIT_CURRENT_YEAR "%Y")
set(QWINDOWKIT_COPYRIGHT "Copyright 2023-${_QWINDOWKIT_CURRENT_YEAR} Stdware Collections")
set(QWINDOWKIT_DESCRIPTION ${PROJECT_DESCRIPTION})

# ----------------------------------
# Find basic dependencies
# ----------------------------------
find_package(qmsetup QUIET)

if(NOT TARGET qmsetup::library)
    # Modify this variable according to your project structure
    set(_source_dir ${CMAKE_CURRENT_SOURCE_DIR}/qmsetup)

    # Import install function
    include("${_source_dir}/cmake/modules/private/InstallPackage.cmake")

    # Install package in place
    set(_package_path)
    qm_install_package(qmsetup
        SOURCE_DIR ${_source_dir}
        BUILD_TYPE Release
        RESULT_PATH _package_path
    )

    # Find package again
    find_package(qmsetup REQUIRED PATHS ${_package_path})

    # Update import path
    set(qmsetup_DIR ${_package_path} CACHE PATH "" FORCE)
endif()

qm_import(Filesystem)
qm_init_directories()

set(QT_NO_PRIVATE_MODULE_WARNING ON)

# ----------------------------------
# Add source modules
# ----------------------------------
add_subdirectory(src)

if(QWINDOWKIT_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()