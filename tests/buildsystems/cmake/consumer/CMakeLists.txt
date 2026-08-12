# The CMake half of the build system regression tests, configured by `RunBuild.cmake` against a
# staged install tree. Nothing in this repository adds this directory: it stands on its own, the
# way a project that consumes the installed package does.

cmake_minimum_required(VERSION 3.19)

project(qwk_cmake_consumer LANGUAGES CXX)

# The staged install tree and Qt, as two variables rather than one CMAKE_PREFIX_PATH, since a list
# does not survive the driver's command line in one piece.
list(APPEND CMAKE_PREFIX_PATH "${QWK_PREFIX}" "${QT_PREFIX}")

set(_components Core)

if(QWK_USE_WIDGETS)
    list(APPEND _components Widgets)
endif()

# Asking by component is part of what is under test. QWindowKitConfig.cmake finds a different set
# of Qt modules for each one.
find_package(QWindowKit REQUIRED COMPONENTS ${_components})

add_executable(qwk_cmake_consumer ../../consumer.cpp)

target_compile_features(qwk_cmake_consumer PRIVATE cxx_std_17)
target_link_libraries(qwk_cmake_consumer PRIVATE QWindowKit::Core)

if(QWK_USE_WIDGETS)
    target_compile_definitions(qwk_cmake_consumer PRIVATE CONSUMER_USE_WIDGETS)
    target_link_libraries(qwk_cmake_consumer PRIVATE QWindowKit::Widgets)
endif()
