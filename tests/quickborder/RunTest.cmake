execute_process(COMMAND "${TEST_EXE}" ${TEST_FUNCTIONS} -o "${RESULT_FILE},txt"
    RESULT_VARIABLE _result TIMEOUT 45)
if(EXISTS "${RESULT_FILE}")
    file(READ "${RESULT_FILE}" _report)
    message("${_report}")
endif()
if(NOT "${_result}" STREQUAL "0")
    message(FATAL_ERROR "Quick border regression failed: ${_result}")
endif()
