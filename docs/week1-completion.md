# Week 1 Completion Report

## Outcome

Week 1 establishes the C/C++ frontend foundation without moving or replacing
the academic compiler. The C/C++ product is primary in the documentation; the
mini-language compiler remains a separate build and test project.

## Delivered

1. Repository audit: [repository-audit.md](repository-audit.md).
2. Dual-frontend architecture: [architecture.md](architecture.md).
3. Explicit subset: [cpp-subset-spec.md](cpp-subset-spec.md).
4. Standalone frontend at `compiler/cpp-frontend/` with C++17 and Flex.
5. Independent `optiq_cpp_lexer` and `optiq_cpp` targets.
6. Root Makefile targets for mini-only, C/C++-only, and combined verification.
7. README with C/C++ as the primary supported source language.
8. Development roadmap separating current lexer work from future parser phases.

## Compatibility decisions

- Existing academic sources and `compiler/CMakeLists.txt` are unchanged.
- No mini-language files were moved, renamed, or deleted.
- The C/C++ implementation uses `optiq::cpp`, separate generated Flex output,
  and a separate executable, avoiding scanner-symbol and data-model coupling.
- No parser, AST, semantic analyzer, TAC generator, optimizer, backend, or
  external test framework was introduced.

## Executed baseline verification

Environment: GNU C++ 12.2.0, Flex 2.6.4, Bison 3.8.2, CMake 3.25.1, Linux.

| Test | Input | Expected | Actual | Result |
| --- | --- | --- | --- | --- |
| Academic clean build | Untouched `main` source archive | Configure and compile successfully | Build completed before C/C++ changes | PASS |
| Academic CTest suite | Thirteen existing tests | 13/13 pass | 13/13 passed; 0 failed | PASS |
| Academic golden programs | Eleven `.mc` programs through `example_tests` | Match expected output/error files | `example_tests` passed | PASS |
| Grammar generation | Existing `parser.y` with Bison warnings enabled | No conflict output | No conflict output observed | PASS |

No Windows test was executed. Windows commands are documented but are not
listed as verified.

## Week 1 acceptance

Existing OptiQ functionality is intact, the new architecture and subset are
documented, and both standalone build configurations are present. Final
post-change results, including the C/C++ lexer suite, are recorded in
[verification.md](verification.md).
