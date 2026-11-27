execute_process(
    COMMAND "${CLI}" --parse "${SOURCE_FILE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)
if(NOT result EQUAL 2)
    message(FATAL_ERROR "Expected syntax-error exit 2, got ${result}. Output: ${output}. Error: ${error}")
endif()
foreach(required IN ITEMS "04-syntax-errors.cpp:2:17: syntax error:" "unexpected" ";")
    string(FIND "${error}" "${required}" position)
    if(position EQUAL -1)
        message(FATAL_ERROR "Missing diagnostic '${required}': ${error}")
    endif()
endforeach()
string(FIND "${output}" "Parser: FAILED" position)
if(position EQUAL -1)
    message(FATAL_ERROR "CLI did not report parser failure: ${output}")
endif()
