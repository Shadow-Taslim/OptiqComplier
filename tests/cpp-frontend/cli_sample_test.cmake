execute_process(
    COMMAND "${CLI}" "${SOURCE_FILE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error_output
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "Expected exit 0, got ${result}: ${error_output}")
endif()
if(NOT error_output STREQUAL "")
    message(FATAL_ERROR "Expected no diagnostics, got: ${error_output}")
endif()

foreach(expected IN ITEMS
    "PREPROCESSOR[ \t]+#include <iostream>"
    "IDENTIFIER[ \t]+std"
    "SCOPE[ \t]+::"
    "IDENTIFIER[ \t]+cout"
    "SHIFT_LEFT[ \t]+<<"
    "STRING_LITERAL[ \t]+\"OptiQ Compiler\""
    "IDENTIFIER[ \t]+endl"
)
    if(NOT output MATCHES "${expected}")
        message(FATAL_ERROR "Token output does not match '${expected}': ${output}")
    endif()
endforeach()
