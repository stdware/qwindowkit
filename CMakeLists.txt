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
option(QWINDOWKIT_BUILD_TESTS "Build test cases" OFF)
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

set(QWINDOWKIT_COPYRIGHT "Copyright 2023-present Stdware Collections")
set(QWINDOWKIT_DESCRIPTION ${PROJECT_DESCRIPTION})

# ----------------------------------
# Build Functions
# ----------------------------------
include(cmake/WriteIfDifferent.cmake)
include(cmake/GetImportedLocation.cmake)
include(cmake/ConfigureTarget.cmake)
include(cmake/AddWinRC.cmake)
include(cmake/AddWinManifest.cmake)
include(cmake/AddMacBundle.cmake)
include(cmake/AddLibrary.cmake)
include(cmake/AddInstallPackageRule.cmake)
include(cmake/GenerateConfigHeader.cmake)
include(cmake/SyncHeaders.cmake)
include(cmake/AddDoxygenDocs.cmake)

# ----------------------------------
# Build Output Directories
# ----------------------------------
# Root of the build outputs: binaries, forwarding headers, the configuration header and the
# generated qmake and MSBuild files. A parent project that embeds QWindowKit sets it beforehand to
# collect the outputs in a directory of its own. It is the only way to do so, because the
# CMAKE_*_OUTPUT_DIRECTORY variables of a parent project are not followed.
if(NOT DEFINED QWINDOWKIT_BUILD_DIR)
    set(QWINDOWKIT_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/out")
endif()

# Files are written under it at configure time, where a generator expression is not evaluated and
# would become part of a directory name.
string(GENEX_STRIP "${QWINDOWKIT_BUILD_DIR}" _stripped_build_dir)

if(NOT _stripped_build_dir STREQUAL "${QWINDOWKIT_BUILD_DIR}")
    message(FATAL_ERROR "QWINDOWKIT_BUILD_DIR must not contain generator expressions: "
        "${QWINDOWKIT_BUILD_DIR}")
endif()

# For the examples. The library targets set their output directories themselves.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${QWINDOWKIT_BUILD_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${QWINDOWKIT_BUILD_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${QWINDOWKIT_BUILD_DIR}/lib)

# ----------------------------------
# Qt
# ----------------------------------
# Qt warns about every private module that a project uses unless this is set. QWindowKit uses the
# private modules on purpose.
set(QT_NO_PRIVATE_MODULE_WARNING ON)

# ----------------------------------
# Add source modules
# ----------------------------------
add_subdirectory(src)

if(QWINDOWKIT_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(QWINDOWKIT_BUILD_TESTS)
    enable_testing()

    add_subdirectory(tests)
endif()
