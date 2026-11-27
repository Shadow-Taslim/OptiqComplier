execute_process(
    COMMAND "${CLI}" "${SOURCE_FILE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error_output
)

if(NOT result EQUAL 2)
    message(FATAL_ERROR "Expected lexical-error exit 2, got ${result}")
endif()

foreach(expected IN ITEMS
    "Lexical error at line 2, column 17: Malformed numeric literal: 12.3.4"
    "Lexical error at line 3, column 23: Invalid escape sequence: \\q"
    "Lexical error at line 4, column 24: Invalid character: @"
    "Lexical error at line 5, column 1: Unterminated block comment"
)
    string(FIND "${error_output}" "${expected}" position)
    if(position EQUAL -1)
        message(FATAL_ERROR "Missing diagnostic '${expected}': ${error_output}")
    endif()
endforeach()
