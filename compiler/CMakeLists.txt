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

if(OPTIQ_BUILD_TESTS)
    enable_testing()

    add_executable(optiq_ast_tests
        ../tests/compiler/ast_tests.cpp
    )
    target_link_libraries(optiq_ast_tests PRIVATE optiq_ast)
    add_test(NAME ast_tests COMMAND optiq_ast_tests)
endif()
