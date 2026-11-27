cmake_minimum_required(VERSION 3.20)

project(OptiqCppFrontend VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(OPTIQ_CPP_BUILD_TESTS "Build OptiQ C/C++ frontend tests" ON)

find_package(FLEX 2.6 REQUIRED)
find_package(BISON 3.8 REQUIRED)
FLEX_TARGET(CppLexer src/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/cpp_lexer.cpp)
BISON_TARGET(CppParser src/parser.y ${CMAKE_CURRENT_BINARY_DIR}/cpp_parser.cpp
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/cpp_parser.hpp
    COMPILE_FLAGS "-Wall")
set_source_files_properties(${FLEX_CppLexer_OUTPUTS} PROPERTIES LANGUAGE CXX)

add_library(optiq_cpp_lexer
    src/token.cpp
    src/scanner_state.cpp
    src/lexer.cpp
    ${FLEX_CppLexer_OUTPUTS}
)
target_include_directories(optiq_cpp_lexer
    PUBLIC include
    PRIVATE src ${CMAKE_CURRENT_BINARY_DIR}
)

add_library(optiq_cpp_parser
    src/ast.cpp
    src/ast_printer.cpp
    src/parser.cpp
    ${BISON_CppParser_OUTPUTS}
)
target_include_directories(optiq_cpp_parser
    PUBLIC include
    PRIVATE src ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(optiq_cpp_parser PUBLIC optiq_cpp_lexer)

add_executable(optiq_cpp src/cli.cpp)
target_link_libraries(optiq_cpp PRIVATE optiq_cpp_parser)

if(OPTIQ_CPP_BUILD_TESTS)
    enable_testing()

    add_executable(optiq_cpp_lexer_tests ../../tests/cpp-frontend/lexer_tests.cpp)
    target_link_libraries(optiq_cpp_lexer_tests PRIVATE optiq_cpp_lexer)

    foreach(test_name IN ITEMS
        keywords
        identifiers
        integer_literals
        floating_literals
        character_literals
        string_literals
        arithmetic_operators
        relational_operators
        logical_operators
        assignment_operators
        increment_decrement
        bitwise_operators
        delimiters
        comments
        preprocessor
        invalid_characters
        malformed_literals
        source_locations
    )
        add_test(NAME cpp_${test_name} COMMAND optiq_cpp_lexer_tests ${test_name})
    endforeach()

    add_executable(optiq_cpp_parser_tests ../../tests/cpp-frontend/parser_tests.cpp)
    target_link_libraries(optiq_cpp_parser_tests PRIVATE optiq_cpp_parser)

    foreach(test_name IN ITEMS
        positive_empty_translation_unit
        positive_main_function
        positive_variable_declarations
        positive_variable_initialization
        positive_arithmetic_expressions
        positive_relational_expressions
        positive_logical_expressions
        positive_nested_expressions
        positive_if_statement
        positive_if_else_statement
        positive_while_loop
        positive_for_loop
        positive_do_while_loop
        positive_function_definition
        positive_function_call
        positive_return_statement
        positive_nested_blocks
        positive_increment_decrement
        positive_cout_syntax
        positive_cin_syntax
        positive_qualified_stream_syntax
        positive_include_directives
        positive_namespace_declaration
        negative_missing_semicolon
        negative_missing_closing_brace
        negative_missing_opening_parenthesis
        negative_invalid_expression
        negative_incomplete_assignment
        negative_invalid_function_declaration
        negative_incorrect_parameter_syntax
        negative_invalid_for_loop
        negative_invalid_stream_insertion
        negative_unexpected_token
        negative_malformed_nested_expression
        negative_invalid_control_flow
        operator_precedence
        assignment_associativity
        ast_source_locations
        error_recovery
        type_restrictions
    )
        add_test(NAME cpp_parser_${test_name} COMMAND optiq_cpp_parser_tests ${test_name})
    endforeach()

    add_test(NAME cpp_cli_sample
        COMMAND ${CMAKE_COMMAND}
            -DCLI=$<TARGET_FILE:optiq_cpp>
            -DSOURCE_FILE=${PROJECT_SOURCE_DIR}/../../examples/cpp-subset/01-full.cpp
            -P ${PROJECT_SOURCE_DIR}/../../tests/cpp-frontend/cli_sample_test.cmake
    )

    add_test(NAME cpp_cli_invalid
        COMMAND ${CMAKE_COMMAND}
            -DCLI=$<TARGET_FILE:optiq_cpp>
            -DSOURCE_FILE=${PROJECT_SOURCE_DIR}/../../examples/cpp-subset/02-lexical-errors.cpp
            -P ${PROJECT_SOURCE_DIR}/../../tests/cpp-frontend/cli_invalid_test.cmake
    )

    add_test(NAME cpp_cli_parse_sample
        COMMAND ${CMAKE_COMMAND}
            -DCLI=$<TARGET_FILE:optiq_cpp>
            -DSOURCE_FILE=${PROJECT_SOURCE_DIR}/../../examples/cpp-subset/03-parser-full.cpp
            -P ${PROJECT_SOURCE_DIR}/../../tests/cpp-frontend/cli_parse_test.cmake
    )

    add_test(NAME cpp_cli_ast_sample
        COMMAND ${CMAKE_COMMAND}
            -DCLI=$<TARGET_FILE:optiq_cpp>
            -DSOURCE_FILE=${PROJECT_SOURCE_DIR}/../../examples/cpp-subset/03-parser-full.cpp
            -P ${PROJECT_SOURCE_DIR}/../../tests/cpp-frontend/cli_ast_test.cmake
    )

    add_test(NAME cpp_cli_syntax_error
        COMMAND ${CMAKE_COMMAND}
            -DCLI=$<TARGET_FILE:optiq_cpp>
            -DSOURCE_FILE=${PROJECT_SOURCE_DIR}/../../examples/cpp-subset/04-syntax-errors.cpp
            -P ${PROJECT_SOURCE_DIR}/../../tests/cpp-frontend/cli_syntax_error_test.cmake
    )

    add_test(NAME cpp_cli_help COMMAND optiq_cpp --help)
    set_tests_properties(cpp_cli_help PROPERTIES
        PASS_REGULAR_EXPRESSION "Usage: optiq_cpp"
    )
endif()
