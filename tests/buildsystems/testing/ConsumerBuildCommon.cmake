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

#[[
    Run one step of a consumer build in WORK_DIR, and turn a non-zero exit into a test failure
    carrying the command and its output. The output is held back until then, since a passing test
    that prints a whole build log buries the ones that did not pass.

    qwk_run_step(<label> <command> [<arg...>])
]] #
function(qwk_run_step _label)
    execute_process(
        COMMAND ${ARGN}
        WORKING_DIRECTORY "${WORK_DIR}"
        RESULT_VARIABLE _code
        OUTPUT_VARIABLE _out
        ERROR_VARIABLE _err
    )

    if(NOT _code EQUAL 0)
        message(FATAL_ERROR
            "${_label} failed with exit code ${_code}\n"
            "command: ${ARGN}\n"
            "--- output ---\n${_out}\n${_err}"
        )
    endif()
endfunction()
