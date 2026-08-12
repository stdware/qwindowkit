# Configures and builds the CMake consumer project against a staged install tree.
#
# cmake -D SOURCE_DIR=... -D INSTALL_PREFIX=... -D QT_PREFIX=... -D WORK_DIR=...
#       -D BUILD_CONFIG=Debug -D GENERATOR=... -D MAKE_PROGRAM=... -D CXX_COMPILER=...
#       -D USE_WIDGETS=ON|OFF -P cmake/RunBuild.cmake

include("${CMAKE_CURRENT_LIST_DIR}/../testing/ConsumerBuildCommon.cmake")

qwk_require_variables(SOURCE_DIR INSTALL_PREFIX QT_PREFIX WORK_DIR BUILD_CONFIG GENERATOR
    CXX_COMPILER USE_WIDGETS
)

file(REMOVE_RECURSE "${WORK_DIR}")
file(MAKE_DIRECTORY "${WORK_DIR}")

# The generator, the make program and the compiler are the ones this build is using. A consumer
# built by a different toolchain than the library it links is a question about ABIs rather than
# about the package files under test, and on Windows CMake would otherwise pick a Visual Studio
# generator here whatever the outer build was.
set(_toolchain_args -G "${GENERATOR}" "-DCMAKE_CXX_COMPILER=${CXX_COMPILER}")

if(MAKE_PROGRAM)
    list(APPEND _toolchain_args "-DCMAKE_MAKE_PROGRAM=${MAKE_PROGRAM}")
endif()

qwk_run_step("cmake configure" "${CMAKE_COMMAND}"
    -S "${SOURCE_DIR}"
    -B "${WORK_DIR}/build"
    ${_toolchain_args}
    "-DCMAKE_BUILD_TYPE=${BUILD_CONFIG}"
    "-DQWK_PREFIX=${INSTALL_PREFIX}"
    "-DQT_PREFIX=${QT_PREFIX}"
    "-DQWK_USE_WIDGETS=${USE_WIDGETS}"
)

qwk_run_step("cmake build" "${CMAKE_COMMAND}"
    --build "${WORK_DIR}/build"
    --config "${BUILD_CONFIG}"
)
