if(NOT DEFINED CLI OR NOT DEFINED SQL_FILE)
    message(FATAL_ERROR "CLI and SQL_FILE must be provided")
endif()

execute_process(
    COMMAND "${CLI}"
    INPUT_FILE "${SQL_FILE}"
    RESULT_VARIABLE status
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)

if(NOT status EQUAL 0)
    message(FATAL_ERROR "CLI stdin mode exited with ${status}: ${error}")
endif()
if(NOT output MATCHES "SelectStatement")
    message(FATAL_ERROR "CLI stdin mode did not print an AST: ${output}")
endif()
