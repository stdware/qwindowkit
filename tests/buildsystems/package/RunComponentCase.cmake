cmake_minimum_required(VERSION 3.19)
include("${CMAKE_CURRENT_LIST_DIR}/../testing/ConsumerBuildCommon.cmake")
qwk_require_variables(SOURCE_DIR INSTALL_PREFIX QT_PREFIX WORK_DIR BUILD_CONFIG GENERATOR
    CXX_COMPILER USE_WIDGETS USE_QUICK QT_MAJOR CASE)
file(MAKE_DIRECTORY "${WORK_DIR}")
qwk_find_installed_config(package_dir "${INSTALL_PREFIX}")
file(REMOVE "${WORK_DIR}/build/CMakeCache.txt")
set(toolchain_args -G "${GENERATOR}" "-DCMAKE_CXX_COMPILER=${CXX_COMPILER}")
if(MAKE_PROGRAM)
    list(APPEND toolchain_args "-DCMAKE_MAKE_PROGRAM=${MAKE_PROGRAM}")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}"
    -S "${SOURCE_DIR}" -B "${WORK_DIR}/build" ${toolchain_args}
    "-DCMAKE_BUILD_TYPE=${BUILD_CONFIG}" "-DPACKAGE_DIR=${package_dir}"
    "-DQT_PREFIX=${QT_PREFIX}" "-DUSE_WIDGETS=${USE_WIDGETS}" "-DUSE_QUICK=${USE_QUICK}"
    "-DQT_MAJOR=${QT_MAJOR}" "-DCASE=${CASE}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 15)
if(CASE MATCHES "^(missing-required|unknown-required)$")
    if(NOT "${result}" MATCHES "^[1-9][0-9]*$" OR
       NOT "${output}${error}" MATCHES "Unavailable QWindowKit components:")
        message(FATAL_ERROR "Expected package-level component failure, got ${result}\n${output}\n${error}")
    endif()
    message(STATUS "${CASE}: rejected during package discovery with a component diagnostic")
elseif(NOT "${result}" STREQUAL "0")
    message(FATAL_ERROR "${CASE}: configuration failed (${result})\n${output}\n${error}")
elseif(NOT CASE STREQUAL "quiet-missing")
    qwk_run_step("component consumer build" 15 "${CMAKE_COMMAND}"
        --build "${WORK_DIR}/build" --config "${BUILD_CONFIG}")
    message(STATUS "${CASE}: component states, exports and consumer link passed")
endif()
