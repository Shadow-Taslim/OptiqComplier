execute_process(
    COMMAND "${CLI}" --ast "${SOURCE_FILE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Expected AST exit 0, got ${result}. Error: ${error}")
endif()
foreach(required IN ITEMS "Program @1:1" "FunctionDefinition int add" "FunctionDefinition int main" "ForStatement" "DoWhileStatement" "StreamOutput std::cout" "StreamInput std::cin")
    string(FIND "${output}" "${required}" position)
    if(position EQUAL -1)
        message(FATAL_ERROR "Missing AST output '${required}': ${output}")
    endif()
endforeach()
