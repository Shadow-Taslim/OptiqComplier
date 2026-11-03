cmake_minimum_required(VERSION 3.20)

project(OptiqCompiler VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(OPTIQ_BUILD_TESTS "Build OptiqCompiler tests" ON)

# Week 1: project configuration only. Library, CLI, Flex/Bison, and test
# targets will be added with their source files in later iterations.
