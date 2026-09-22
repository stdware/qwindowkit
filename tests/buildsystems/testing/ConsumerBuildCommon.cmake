# Shared by the consumer build drivers, which run under `cmake -P` and therefore have none of the
# project's own variables or functions. Nothing in this directory is a test, it is what the tests
# next to it are written against.

#[[
    Abort unless every named variable arrived with a value.

    qwk_require_variables(<var...>)
]] #
function(qwk_require_variables)
    foreach(_var IN LISTS ARGN)
        if(NOT DEFINED ${_var} OR "${${_var}}" STREQUAL "")
            message(FATAL_ERROR "${_var} was not passed to ${CMAKE_CURRENT_LIST_FILE}.")
        endif()
    endforeach()
endfunction()

function(qwk_find_installed_config _out _prefix)
    file(GLOB_RECURSE _configs "${_prefix}/*QWindowKitConfig.cmake")
    list(LENGTH _configs _count)
    if(NOT _count EQUAL 1)
        message(FATAL_ERROR "Expected one installed QWindowKit config in ${_prefix}, got ${_configs}")
    endif()
    list(GET _configs 0 _config)
    get_filename_component(_directory "${_config}" DIRECTORY)
    set(${_out} "${_directory}" PARENT_SCOPE)
endfunction()

#[[
    Run one step of a consumer build in WORK_DIR, and turn a non-zero exit into a test failure
    carrying the command and its output. The output is held back until then, since a passing test
    that prints a whole build log buries the ones that did not pass.

    qwk_run_step(<label> <timeout-seconds> <command> [<arg...>])
]] #
function(qwk_run_step _label _timeout)
    if(NOT _timeout MATCHES "^[1-9][0-9]*$" OR _timeout GREATER 25)
        message(FATAL_ERROR "Consumer step timeout must be an integer between 1 and 25 seconds")
    endif()
    execute_process(
        COMMAND ${ARGN}
        WORKING_DIRECTORY "${WORK_DIR}"
        RESULT_VARIABLE _code
        OUTPUT_VARIABLE _out
        ERROR_VARIABLE _err
        TIMEOUT ${_timeout}
    )

    if(NOT "${_code}" STREQUAL "0")
        message(FATAL_ERROR
            "${_label} failed with exit code ${_code}\n"
            "command: ${ARGN}\n"
            "--- output ---\n${_out}\n${_err}"
        )
    endif()
endfunction()
