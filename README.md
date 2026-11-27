# OptiQ — C/C++ Subset Compiler

OptiQ's primary product is a C++17 compiler frontend for a clearly defined
common subset of C and C++. The repository also preserves the original academic
mini-language compiler as an independent implementation.

> **Current status:** Weeks 1–2 provide the C/C++ architecture, explicit subset
> specification, Flex lexical analyzer, diagnostics, token-table CLI, examples,
> and automated tests. C/C++ parsing, semantic analysis, TAC, optimization, and
> executable generation are not implemented yet.

The original `.mc` compiler remains fully available with its Flex/Bison parser,
AST, semantic analysis, and Three Address Code output. Its pre-migration source
is also preserved on the `academic-baseline` branch.

## C/C++ Example

```cpp
#include <iostream>

int main() {
    int x = 10;
    double price = 25.50;
    bool active = true;
    x++;

    if (x >= 10 && active) {
        std::cout << "OptiQ Compiler" << std::endl;
    }
    return 0;
}
```

The lexer recognizes the documented keywords, identifiers, decimal literals,
characters, strings, operators, delimiters, comments, and limited preprocessor
lines. In the example above, `std`, `::`, `cout`, `<<`, and `endl` are tokenized
correctly; this does not claim that the complete program is parsed or executed.
See [docs/cpp-subset-spec.md](docs/cpp-subset-spec.md) for exact limits.

## Build and Test

Dependencies: CMake 3.20+, a C++17 compiler (GCC/MinGW-w64), and Flex 2.6+.
Bison 3.0+ is also required for the academic mini-language build. On Windows,
use WinFlexBison (`win_flex` / `win_bison`) on `PATH`.

Linux/macOS:

```sh
make cpp         # C/C++ frontend only
make test-cpp    # C/C++ lexer and CLI tests
make mini        # Academic mini compiler only
make test-mini   # Academic regression tests
make test        # Both independent suites
```

Windows PowerShell — C/C++ frontend:

```powershell
cmake -S compiler/cpp-frontend -B build/cpp-frontend -G "MinGW Makefiles" -DOPTIQ_CPP_BUILD_TESTS=ON
cmake --build build/cpp-frontend --parallel
ctest --test-dir build/cpp-frontend --output-on-failure
```

Windows PowerShell — academic compiler:

```powershell
cmake -S compiler -B build/compiler -G "MinGW Makefiles" -DOPTIQ_BUILD_TESTS=ON
cmake --build build/compiler --parallel
ctest --test-dir build/compiler --output-on-failure
```

Generated scanner/parser sources remain in the build directories and are never
committed. The two frontends can be configured and tested independently.

## Running the C/C++ Lexer

```sh
./build/cpp-frontend/optiq_cpp examples/cpp-subset/01-full.cpp
./build/cpp-frontend/optiq_cpp --tokens examples/cpp-subset/01-full.cpp
```

PowerShell:

```powershell
.\build\cpp-frontend\optiq_cpp.exe .\examples\cpp-subset\01-full.cpp
Get-Content .\examples\cpp-subset\01-full.cpp -Raw | .\build\cpp-frontend\optiq_cpp.exe
```

Output begins:

```text
TOKEN TYPE            LEXEME                          LINE      COLUMN
PREPROCESSOR          #include <iostream>             1         1
KEYWORD               int                             3         1
IDENTIFIER            main                            3         5
LEFT_PAREN            (                               3         9
RIGHT_PAREN           )                               3         10
```

The complete output is in
[examples/cpp-subset/01-full.tokens.txt](examples/cpp-subset/01-full.tokens.txt).
The invalid example returns exit code `2` and reports errors such as:

```text
Lexical error at line 2, column 17: Malformed numeric literal: 12.3.4
Lexical error at line 3, column 23: Invalid escape sequence: \q
```

C/C++ CLI exit codes: `0` tokenized · `2` lexical error · `1` command, I/O, or
internal error.

## Academic Mini-Language Compiler

The mini language supports `int`, `float`, and `bool`; declarations,
assignments, `if`/`else`, `while`, `print`, nested blocks, expressions,
comments, semantic checks, and TAC generation. Its authoritative grammar and
semantic rules remain in [docs/language-spec.md](docs/language-spec.md).

```sh
./build/compiler/optiq_cli examples/06-manual-sample.mc
./build/compiler/optiq_cli --tokens examples/01-variables.mc
./build/compiler/optiq_cli --ast examples/03-if-else.mc
./build/compiler/optiq_cli --json examples/03-if-else.mc
./build/compiler/optiq_cli --check examples/04-while-loop.mc
```

Mini CLI exit codes remain `0` compiled · `2` lexical/syntax error · `3`
semantic error · `1` internal error.

## Repository Structure

```text
├── compiler/
│   ├── CMakeLists.txt             # Standalone academic mini compiler
│   ├── include/optiq/             # Academic AST/parser/semantic/TAC interfaces
│   ├── src/                       # Academic Flex/Bison implementation
│   └── cpp-frontend/              # Primary C/C++ frontend
│       ├── CMakeLists.txt         # Standalone C++17/Flex build
│       ├── include/optiq/cpp/     # Tokens, diagnostics, lexer interface
│       └── src/                   # Flex rules, scanner, token names, CLI
├── tests/
│   ├── compiler/                  # Existing academic unit/golden tests
│   ├── programs/                  # Eleven academic test programs
│   ├── expected/                  # Academic golden outputs
│   └── cpp-frontend/              # Eighteen C/C++ lexer test categories
├── examples/
│   ├── *.mc                       # Six academic examples
│   └── cpp-subset/                # C/C++ valid/error examples and token output
├── docs/
│   ├── cpp-subset-spec.md         # Primary C/C++ subset and limitations
│   ├── architecture.md            # Dual-frontend architecture
│   ├── repository-audit.md        # Existing implementation and migration risks
│   ├── development-roadmap.md     # Current and future phases
│   ├── language-spec.md           # Academic mini-language grammar
│   ├── verification.md            # Executed build/test records
│   ├── week1-completion.md        # Foundation report
│   ├── week2-completion.md        # Lexer report with every test result
│   └── project-report.md          # Original academic report
├── Makefile                       # Independent and combined build targets
└── README.md
```

## Verification

On the recorded Linux environment, the migration executed **13/13 academic
CTest tests** and **21/21 C/C++ CTest tests**, all passing. The C/C++ suite
includes eighteen requested lexer categories and three CLI tests. See
[docs/verification.md](docs/verification.md) and
[docs/week2-completion.md](docs/week2-completion.md). Windows commands are
provided but are not claimed as executed in that record.

## Branch Preservation

- `academic-baseline` freezes the original academic implementation and history.
- `main` becomes the primary C/C++ product only after the migration is tested
  and approved.
- `sql-prototype` is unrelated to this migration and remains untouched.
- No branch history is rewritten and no existing branch is deleted.

## Documentation

- [Repository audit](docs/repository-audit.md)
- [Architecture](docs/architecture.md)
- [C/C++ subset specification](docs/cpp-subset-spec.md)
- [Development roadmap](docs/development-roadmap.md)
- [Week 1 report](docs/week1-completion.md)
- [Week 2 report](docs/week2-completion.md)
- [Academic project report](docs/project-report.md)

The academic project report describes the original mini-language submission; it
does not claim C/C++ frontend features.

## Out of Scope in Weeks 1–2

Complete C or C++, full preprocessing, C/C++ parsing and AST construction,
semantic analysis, TAC, optimization, machine code, linking, and executables.

## License

MIT
