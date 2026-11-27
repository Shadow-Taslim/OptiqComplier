execute_process(
    COMMAND "${CLI}" --parse "${SOURCE_FILE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Expected parse exit 0, got ${result}. Output: ${output}. Error: ${error}")
endif()
if(NOT error STREQUAL "")
    message(FATAL_ERROR "Successful parse wrote to stderr: ${error}")
endif()
foreach(required IN ITEMS "Lexer: SUCCESS" "Parser: SUCCESS" "AST: GENERATED" "Syntax analysis completed successfully.")
    string(FIND "${output}" "${required}" position)
    if(position EQUAL -1)
        message(FATAL_ERROR "Missing CLI output '${required}': ${output}")
    endif()
endforeach()
