# Convenience wrapper around the CMake build. Generated Flex/Bison sources
# stay inside build/ and are never committed.

BUILD_DIR := build/compiler

.PHONY: all build test clean

all: build

build:
	cmake -S compiler -B $(BUILD_DIR) -DOPTIQ_BUILD_TESTS=ON
	cmake --build $(BUILD_DIR) --parallel

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

clean:
	rm -rf build
