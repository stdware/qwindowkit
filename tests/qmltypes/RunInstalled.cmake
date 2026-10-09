function(run_step name timeout)
    if(NOT timeout MATCHES "^[1-9][0-9]*$" OR timeout GREATER 20)
        message(FATAL_ERROR "QML consumer step timeout must be an integer between 1 and 20 seconds")
    endif()
    execute_process(COMMAND ${ARGN} RESULT_VARIABLE result
        OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT ${timeout})
    if(NOT "${result}" STREQUAL "0")
        message(FATAL_ERROR "${name} failed (${result})\n${output}\n${error}")
    endif()
endfunction()

function(run_lint import_path expect_success)
    execute_process(COMMAND "${QT_QMLLINT}" --ignore-settings --json -
        -I "${import_path}" "${SOURCE_DIR}/Window.qml"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 5)
    if(NOT "${result}" MATCHES "^[0-9]+$")
        message(FATAL_ERROR "QML lint did not complete (${result})\n${output}\n${error}")
    endif()
    # Newer qmllint versions can return zero even with warnings. Check the report as well.
    string(JSON success GET "${output}" files 0 success)
    if(expect_success)
        if(NOT result EQUAL 0 OR NOT success)
            message(FATAL_ERROR "Installed QML lint failed (${result})\n${output}\n${error}")
        endif()
    elseif(success OR NOT output MATCHES "WindowAgent")
        message(FATAL_ERROR "Negative control did not diagnose missing WindowAgent metadata\n${output}")
    endif()
endfunction()

set(prefix "${WORK_DIR}/prefix")
file(REMOVE "${prefix}/qml/QWindowKit/qmldir" "${prefix}/qml/QWindowKit/QWKQuick.qmltypes")
run_step(install 10 "${CMAKE_COMMAND}" --install "${BUILD_DIR}"
    --prefix "${prefix}" --config "${BUILD_CONFIG}")
foreach(file qmldir QWKQuick.qmltypes)
    if(NOT EXISTS "${prefix}/qml/QWindowKit/${file}")
        message(FATAL_ERROR "Installed QML metadata is missing: ${file}")
    endif()
endforeach()

# Only the staged installation is on the custom import path, never the build tree.
run_lint("${prefix}/qml" TRUE)

set(generator_args -G "${GENERATOR}")
if(GENERATOR_PLATFORM)
    list(APPEND generator_args -A "${GENERATOR_PLATFORM}")
endif()
if(GENERATOR_TOOLSET)
    list(APPEND generator_args -T "${GENERATOR_TOOLSET}")
endif()
run_step(configure 20 "${CMAKE_COMMAND}" -S "${SOURCE_DIR}" -B "${WORK_DIR}/build"
    ${generator_args} "-DCMAKE_PREFIX_PATH=${prefix}" "-DQT_DIR=${QT_DIR}" "-DQt6_DIR=${QT_DIR}"
    "-DCMAKE_BUILD_TYPE=${BUILD_CONFIG}")
run_step(build 20 "${CMAKE_COMMAND}" --build "${WORK_DIR}/build" --config "${BUILD_CONFIG}")
file(READ "${WORK_DIR}/build/consumer-${BUILD_CONFIG}.txt" consumer)
set(ENV{PATH} "${prefix}/${INSTALL_BINDIR};${QT_BIN_DIR};$ENV{PATH}")
set(ENV{QT_QPA_PLATFORM} windows)
set(ENV{QT_QUICK_BACKEND} software)
run_step(runtime 8 "${consumer}" "${SOURCE_DIR}/Window.qml" "${prefix}/qml")

# Prove that the lint check needs our type information, rather than passing on imports alone.
file(MAKE_DIRECTORY "${WORK_DIR}/without-types/QWindowKit")
file(WRITE "${WORK_DIR}/without-types/QWindowKit/qmldir" "module QWindowKit\n")
run_lint("${WORK_DIR}/without-types" FALSE)
message(STATUS "Installed QML metadata, lint, runtime registration and negative control passed")
