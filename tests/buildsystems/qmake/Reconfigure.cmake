cmake_minimum_required(VERSION 3.19)

include("${CMAKE_CURRENT_LIST_DIR}/../testing/ConsumerBuildCommon.cmake")
qwk_require_variables(SOURCE_DIR TEST_ROOT BUILD_CONFIG STAGE GENERATOR QT_PREFIX QMSETUP_DIR
    QMAKE_EXECUTABLE MAKE_PROGRAM)
if(NOT BUILD_CONFIG MATCHES "^(Debug|Release|RelWithDebInfo|MinSizeRel)$" OR
   NOT STAGE MATCHES "^(prepare|build_core|build_widgets|build_quick|all|core|widgets|quick|restored)$")
    message(FATAL_ERROR "Unexpected reconfiguration stage/configuration")
endif()

# Resolve the generated work directory before any cleanup. Never clean the caller's
# project/build root, or follow a work-directory junction outside that root.
get_filename_component(_test_root "${TEST_ROOT}" REALPATH)
set(_root "${_test_root}/qmake-reconfigure-${BUILD_CONFIG}")
get_filename_component(_resolved "${_root}" REALPATH)
if(NOT _resolved STREQUAL _root)
    message(FATAL_ERROR "Reconfiguration work directory resolves elsewhere: ${_resolved}")
endif()
if(STAGE STREQUAL "prepare")
    file(REMOVE_RECURSE "${_root}")
endif()
file(MAKE_DIRECTORY "${_root}")
set(WORK_DIR "${_root}")
set(_build "${_root}/build")
if(STAGE MATCHES "^build_")
    # A real cold library build gets its own deadline, separate from configuration
    # and consumption. No substitutes for the libraries or their install rules.
    set(_build_core QWKCore)
    set(_build_widgets QWKWidgets)
    set(_build_quick QWKQuick)
    execute_process(COMMAND "${CMAKE_COMMAND}" --build "${_build}"
        --config "${BUILD_CONFIG}" --parallel 4 --target ${_${STAGE}}
        RESULT_VARIABLE _result OUTPUT_VARIABLE _out ERROR_VARIABLE _err TIMEOUT 50)
    if(NOT "${_result}" STREQUAL "0")
        message(FATAL_ERROR "Initial library build failed (${_result}):\n${_out}\n${_err}")
    endif()
    return()
endif()
set(_prefix "${_root}/install-${STAGE}")
set(_widgets OFF)
set(_quick OFF)
set(_modules core)
set(_targets QWKCore)
if(STAGE MATCHES "^(prepare|all|widgets|restored)$")
    set(_widgets ON)
    list(APPEND _modules widgets)
    list(APPEND _targets QWKWidgets)
endif()
if(STAGE MATCHES "^(prepare|all|quick|restored)$")
    set(_quick ON)
    list(APPEND _modules quick)
    list(APPEND _targets QWKQuick)
endif()

set(_generator_args -G "${GENERATOR}")
if(GENERATOR_PLATFORM)
    list(APPEND _generator_args -A "${GENERATOR_PLATFORM}")
endif()
if(GENERATOR_TOOLSET)
    list(APPEND _generator_args -T "${GENERATOR_TOOLSET}")
endif()
if(CMAKE_MAKE_TOOL)
    list(APPEND _generator_args "-DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_TOOL}")
endif()
if(CXX_COMPILER)
    list(APPEND _generator_args "-DCMAKE_CXX_COMPILER=${CXX_COMPILER}")
endif()
if(DEFINED ENABLE_STYLE_AGENT)
    list(APPEND _generator_args "-DQWINDOWKIT_ENABLE_STYLE_AGENT=${ENABLE_STYLE_AGENT}")
endif()
qwk_run_step("configure ${STAGE}" 15 "${CMAKE_COMMAND}" -S "${SOURCE_DIR}" -B "${_build}"
    ${_generator_args} "-DCMAKE_PREFIX_PATH=${QT_PREFIX}" "-Dqmsetup_DIR=${QMSETUP_DIR}"
    "-DQT_VERSION_MAJOR=${QT_MAJOR}" "-DCMAKE_BUILD_TYPE=${BUILD_CONFIG}"
    "-DCMAKE_DEBUG_POSTFIX=${DEBUG_POSTFIX}"
    "-DQWINDOWKIT_BUILD_STATIC=${BUILD_STATIC}"
    "-DQWINDOWKIT_BUILD_WIDGETS=${_widgets}" "-DQWINDOWKIT_BUILD_QUICK=${_quick}"
    -DQWINDOWKIT_BUILD_TESTS=OFF -DQWINDOWKIT_BUILD_EXAMPLES=OFF
    -DQWINDOWKIT_BUILD_DOCUMENTATIONS=OFF -DQWINDOWKIT_INSTALL=ON
    "-DCMAKE_INSTALL_PREFIX=${_prefix}"
    -DCMAKE_INSTALL_DATADIR=custom/data -DCMAKE_INSTALL_LIBDIR=custom/lib
    -DCMAKE_INSTALL_BINDIR=custom/bin -DCMAKE_INSTALL_INCLUDEDIR=custom/include)

if(STAGE STREQUAL "prepare")
    return()
endif()

qwk_run_step("build ${STAGE}" 15 "${CMAKE_COMMAND}" --build "${_build}"
    --config "${BUILD_CONFIG}" --parallel 4 --target ${_targets})
# Every stage installs into a fresh directory. Preserve stale files in the shared
# build output: they are the regression trigger, and must not be removed by a fix.
if(EXISTS "${_prefix}")
    message(FATAL_ERROR "Expected a new install prefix: ${_prefix}")
endif()
qwk_run_step("install ${STAGE}" 10 "${CMAKE_COMMAND}" --install "${_build}"
    --config "${BUILD_CONFIG}" --prefix "${_prefix}")
set(_pri_dir "${_prefix}/custom/data/QWindowKit/qmake")
file(GLOB _actual RELATIVE "${_pri_dir}" "${_pri_dir}/*.pri")
set(_expected)
foreach(_target IN LISTS _targets)
    list(APPEND _expected "${_target}.pri")
endforeach()
list(SORT _actual)
list(SORT _expected)
if(NOT _actual STREQUAL _expected)
    message(FATAL_ERROR "${STAGE}: expected pri files '${_expected}', got '${_actual}'")
endif()
foreach(_target QWKCore QWKWidgets QWKQuick)
    # Include versioned Unix shared libraries, archives, import libraries and DLLs.
    file(GLOB _libraries "${_prefix}/custom/lib/*${_target}*"
        "${_prefix}/custom/bin/*${_target}*")
    if(_target IN_LIST _targets)
        if(NOT _libraries)
            message(FATAL_ERROR "${STAGE}: missing installed library for ${_target}")
        endif()
    elseif(_libraries)
        message(FATAL_ERROR "${STAGE}: disabled module libraries installed: ${_libraries}")
    endif()
endforeach()
if(STAGE STREQUAL "core")
    foreach(_target QWKWidgets QWKQuick)
        if(NOT EXISTS "${_build}/out/share/QWindowKit/qmake/${_target}.pri")
            message(FATAL_ERROR "Missing stale build artifact for regression: ${_target}.pri")
        endif()
    endforeach()
endif()

set(INSTALL_PREFIX "${_prefix}")
set(PROJECT_FILE "${CMAKE_CURRENT_LIST_DIR}/consumer.pro")
set(WORK_DIR "${_root}/consumer-${STAGE}")
set(QWK_QMAKE_DIR "${_pri_dir}")
set(QWK_MODULES "${_modules}")
if(BUILD_CONFIG STREQUAL "Debug")
    set(BUILD_CONFIG debug)
else()
    set(BUILD_CONFIG release)
endif()
include("${CMAKE_CURRENT_LIST_DIR}/RunBuild.cmake")
message(STATUS "${STAGE}: installed exactly ${_expected}; qmake consumer linked successfully")
