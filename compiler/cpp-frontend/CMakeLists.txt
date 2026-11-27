cmake_minimum_required(VERSION 3.20)

project(OptiqCppFrontend VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(OPTIQ_CPP_BUILD_TESTS "Build OptiQ C/C++ frontend tests" ON)

find_package(FLEX 2.6 REQUIRED)
FLEX_TARGET(CppLexer src/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/cpp_lexer.cpp)
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

add_executable(optiq_cpp src/cli.cpp)
target_link_libraries(optiq_cpp PRIVATE optiq_cpp_lexer)

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

    add_test(NAME cpp_cli_help COMMAND optiq_cpp --help)
    set_tests_properties(cpp_cli_help PROPERTIES
        PASS_REGULAR_EXPRESSION "Usage: optiq_cpp"
    )
endif()
