if(NOT DEFINED CLI)
    message(FATAL_ERROR "CLI must be provided")
endif()

set(source_file "${CMAKE_CURRENT_BINARY_DIR}/optiq-stdin-test.oq")
file(WRITE "${source_file}" "int x;\nx = 40 + 2;\nprint(x);\n")
execute_process(
    COMMAND "${CLI}"
    INPUT_FILE "${source_file}"
    RESULT_VARIABLE status
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)

if(NOT status EQUAL 0)
    message(FATAL_ERROR "CLI stdin mode exited with ${status}: ${error}")
endif()
if(NOT output MATCHES "Program")
    message(FATAL_ERROR "CLI stdin mode did not print an AST: ${output}")
endif()
