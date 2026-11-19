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
endif()
