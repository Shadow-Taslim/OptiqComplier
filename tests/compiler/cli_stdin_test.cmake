# Runs optiq_cli with a source file piped through stdin and checks that
# TAC output is produced (exit code 0 and at least one control-flow
# instruction from the sample program).
execute_process(
    COMMAND sh -c "\"${CLI}\" < \"${SOURCE_FILE}\""
    RESULT_VARIABLE status
    OUTPUT_VARIABLE output
    ERROR_VARIABLE errors
)
if(NOT status EQUAL 0)
    message(FATAL_ERROR "optiq_cli failed on stdin input: ${errors}")
endif()
if(NOT output MATCHES "ifFalse")
    message(FATAL_ERROR "optiq_cli produced no TAC for stdin input: ${output}")
endif()
