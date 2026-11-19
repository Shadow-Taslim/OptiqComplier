cmake_minimum_required(VERSION 3.20)

project(OptiqCompiler VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(OPTIQ_BUILD_TESTS "Build OptiqCompiler tests" ON)

add_library(optiq_ast
    src/ast_printer.cpp
)
target_include_directories(optiq_ast PUBLIC include)

find_package(FLEX 2.6 REQUIRED)
FLEX_TARGET(SqlLexer src/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/sql_lexer.cpp)
set_source_files_properties(${FLEX_SqlLexer_OUTPUTS} PROPERTIES LANGUAGE CXX)

add_library(optiq_lexer
    src/lexer.cpp
    src/scanner_state.cpp
    ${FLEX_SqlLexer_OUTPUTS}
)
target_include_directories(optiq_lexer
    PUBLIC include
    PRIVATE src ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(optiq_lexer PUBLIC optiq_ast)

find_package(BISON 3.0 REQUIRED)
BISON_TARGET(SqlParser src/parser.y ${CMAKE_CURRENT_BINARY_DIR}/sql_parser.cpp
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/sql_parser.hpp
    COMPILE_FLAGS "--warnings=all"
)
set_source_files_properties(${BISON_SqlParser_OUTPUTS} PROPERTIES LANGUAGE CXX)

add_library(optiq_parser
    src/parser.cpp
    ${BISON_SqlParser_OUTPUTS}
)
target_include_directories(optiq_parser
    PUBLIC include
    PRIVATE src ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(optiq_parser PUBLIC optiq_lexer)

add_executable(optiq_cli src/cli.cpp)
target_link_libraries(optiq_cli PRIVATE optiq_parser)

if(OPTIQ_BUILD_TESTS)
    enable_testing()

    add_executable(optiq_ast_tests
        ../tests/compiler/ast_tests.cpp
    )
    target_link_libraries(optiq_ast_tests PRIVATE optiq_ast)
    add_test(NAME ast_tests COMMAND optiq_ast_tests)

    add_executable(optiq_lexer_tests
        ../tests/compiler/lexer_tests.cpp
    )
    target_link_libraries(optiq_lexer_tests PRIVATE optiq_lexer)
    add_test(NAME lexer_tests COMMAND optiq_lexer_tests)

    add_executable(optiq_parser_tests
        ../tests/compiler/parser_tests.cpp
    )
    target_link_libraries(optiq_parser_tests PRIVATE optiq_parser)
    add_test(NAME parser_tests COMMAND optiq_parser_tests)

    add_executable(optiq_fixture_tests
        ../tests/compiler/fixture_tests.cpp
    )
    target_link_libraries(optiq_fixture_tests PRIVATE optiq_parser)
    add_test(NAME fixture_tests
        COMMAND optiq_fixture_tests "${PROJECT_SOURCE_DIR}/.."
    )

    add_test(NAME cli_valid
        COMMAND optiq_cli "SELECT name FROM employees WHERE id = 1;"
    )
    set_tests_properties(cli_valid PROPERTIES
        PASS_REGULAR_EXPRESSION "\"type\": \"SelectStatement\""
    )

    add_test(NAME cli_invalid COMMAND optiq_cli "SELECT FROM employees")
    set_tests_properties(cli_invalid PROPERTIES WILL_FAIL TRUE)

    add_test(NAME cli_help COMMAND optiq_cli --help)
    set_tests_properties(cli_help PROPERTIES PASS_REGULAR_EXPRESSION "Usage: optiq_cli")

    add_test(NAME cli_stdin
        COMMAND ${CMAKE_COMMAND}
            -DCLI=$<TARGET_FILE:optiq_cli>
            -DSQL_FILE=${PROJECT_SOURCE_DIR}/../tests/fixtures/01-ordered-projection.sql
            -P ${PROJECT_SOURCE_DIR}/../tests/compiler/cli_stdin_test.cmake
    )
endif()
