cmake_minimum_required(VERSION 3.20)

project(OptiqCompiler VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(OPTIQ_BUILD_TESTS "Build OptiqCompiler tests" ON)

add_library(optiq_ast
    src/ast_printer.cpp
)
target_include_directories(optiq_ast PUBLIC include)

find_package(FLEX 2.6 REQUIRED)
FLEX_TARGET(McLexer src/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/mc_lexer.cpp)
set_source_files_properties(${FLEX_McLexer_OUTPUTS} PROPERTIES LANGUAGE CXX)

add_library(optiq_lexer
    src/lexer.cpp
    src/scanner_state.cpp
    ${FLEX_McLexer_OUTPUTS}
)
target_include_directories(optiq_lexer
    PUBLIC include
    PRIVATE src ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(optiq_lexer PUBLIC optiq_ast)

find_package(BISON 3.0 REQUIRED)
BISON_TARGET(McParser src/parser.y ${CMAKE_CURRENT_BINARY_DIR}/mc_parser.cpp
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/mc_parser.hpp
    COMPILE_FLAGS "--warnings=all"
)
set_source_files_properties(${BISON_McParser_OUTPUTS} PROPERTIES LANGUAGE CXX)

add_library(optiq_parser
    src/parser.cpp
    ${BISON_McParser_OUTPUTS}
)
target_include_directories(optiq_parser
    PUBLIC include
    PRIVATE src ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(optiq_parser PUBLIC optiq_lexer)

add_library(optiq_semantic
    src/symbol_table.cpp
    src/semantic.cpp
)
target_link_libraries(optiq_semantic PUBLIC optiq_ast)

add_library(optiq_tac
    src/tac.cpp
)
target_link_libraries(optiq_tac PUBLIC optiq_ast)

add_executable(optiq_cli src/cli.cpp)
target_link_libraries(optiq_cli PRIVATE optiq_parser optiq_semantic optiq_tac)

if(OPTIQ_BUILD_TESTS)
    enable_testing()

    add_executable(optiq_ast_tests ../tests/compiler/ast_tests.cpp)
    target_link_libraries(optiq_ast_tests PRIVATE optiq_ast)
    add_test(NAME ast_tests COMMAND optiq_ast_tests)

    add_executable(optiq_lexer_tests ../tests/compiler/lexer_tests.cpp)
    target_link_libraries(optiq_lexer_tests PRIVATE optiq_lexer)
    add_test(NAME lexer_tests COMMAND optiq_lexer_tests)

    add_executable(optiq_parser_tests ../tests/compiler/parser_tests.cpp)
    target_link_libraries(optiq_parser_tests PRIVATE optiq_parser)
    add_test(NAME parser_tests COMMAND optiq_parser_tests)

    add_executable(optiq_semantic_tests ../tests/compiler/semantic_tests.cpp)
    target_link_libraries(optiq_semantic_tests PRIVATE optiq_parser optiq_semantic)
    add_test(NAME semantic_tests COMMAND optiq_semantic_tests)

    add_executable(optiq_tac_tests ../tests/compiler/tac_tests.cpp)
    target_link_libraries(optiq_tac_tests PRIVATE optiq_parser optiq_tac)
    add_test(NAME tac_tests COMMAND optiq_tac_tests)

    add_executable(optiq_example_tests ../tests/compiler/example_tests.cpp)
    target_link_libraries(optiq_example_tests
        PRIVATE optiq_parser optiq_semantic optiq_tac optiq_ast
    )
    add_test(NAME example_tests
        COMMAND optiq_example_tests "${PROJECT_SOURCE_DIR}/.."
    )

    add_test(NAME cli_valid
        COMMAND optiq_cli "int x; x = 2 + 3 * 4; print x;"
    )
    set_tests_properties(cli_valid PROPERTIES
        PASS_REGULAR_EXPRESSION "print x"
    )

    add_test(NAME cli_tokens
        COMMAND optiq_cli --tokens "int x = 1;"
    )
    set_tests_properties(cli_tokens PROPERTIES
        PASS_REGULAR_EXPRESSION "identifier \"x\""
    )

    add_test(NAME cli_ast
        COMMAND optiq_cli --ast "bool b = true; if (b) { print b; }"
    )
    set_tests_properties(cli_ast PROPERTIES
        PASS_REGULAR_EXPRESSION "Declaration bool b ="
    )

    add_test(NAME cli_invalid_syntax COMMAND optiq_cli "int x x = 5;")
    set_tests_properties(cli_invalid_syntax PROPERTIES WILL_FAIL TRUE)

    add_test(NAME cli_invalid_semantic COMMAND optiq_cli "x = 5;")
    set_tests_properties(cli_invalid_semantic PROPERTIES WILL_FAIL TRUE)

    add_test(NAME cli_help COMMAND optiq_cli --help)
    set_tests_properties(cli_help PROPERTIES PASS_REGULAR_EXPRESSION "Usage: optiq_cli")

    add_test(NAME cli_stdin
        COMMAND ${CMAKE_COMMAND}
            -DCLI=$<TARGET_FILE:optiq_cli>
            -DSOURCE_FILE=${PROJECT_SOURCE_DIR}/../examples/06-manual-sample.mc
            -P ${PROJECT_SOURCE_DIR}/../tests/compiler/cli_stdin_test.cmake
    )
endif()
