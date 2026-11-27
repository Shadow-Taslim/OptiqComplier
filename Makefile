# Convenience wrapper around the two independent CMake builds. Generated
# Flex/Bison sources stay inside build/ and are never committed.

MINI_BUILD_DIR := build/compiler
CPP_BUILD_DIR := build/cpp-frontend

.PHONY: all build mini cpp test test-mini test-cpp clean

all: build

build: mini cpp

mini:
	cmake -S compiler -B $(MINI_BUILD_DIR) -DOPTIQ_BUILD_TESTS=ON
	cmake --build $(MINI_BUILD_DIR) --parallel

cpp:
	cmake -S compiler/cpp-frontend -B $(CPP_BUILD_DIR) -DOPTIQ_CPP_BUILD_TESTS=ON
	cmake --build $(CPP_BUILD_DIR) --parallel

test: test-mini test-cpp

test-mini: mini
	ctest --test-dir $(MINI_BUILD_DIR) --output-on-failure

test-cpp: cpp
	ctest --test-dir $(CPP_BUILD_DIR) --output-on-failure

clean:
	cmake -E remove_directory build
