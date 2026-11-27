# OptiQ — C/C++ Subset Compiler

OptiQ's primary product is a C++17 compiler frontend for a clearly defined common
subset of C and C++. The repository also preserves the original academic
mini-language compiler as an independent implementation.

> **Current status:** Weeks 1–3 provide the C/C++ architecture, explicit subset
> specification, Flex lexer, Bison parser, preliminary AST, AST printer, located
> diagnostics, CLI modes, examples, and automated tests. Semantic analysis, TAC,
> optimization, and executable generation for C/C++ are not implemented yet.

The original `.mc` compiler remains fully available with its Flex/Bison parser,
AST, semantic analysis, and Three Address Code output. Its pre-migration source
is also preserved on the `academic-baseline` branch.

## C/C++ Example

```cpp
#include <iostream>

int add(int a, int b) {
    return a + b;
}

int main() {
    int result = add(10, 20);
    for (int i = 0; i < 3; i++) {
        result += i;
    }
    std::cout << "Result: " << result << std::endl;
    return 0;
}
```

The frontend tokenizes and parses this program and can generate its preliminary
AST. Supported syntax includes the six documented types, variable declarations,
basic functions and calls, expressions with defined precedence, nested blocks,
`if`/`else`, `while`, `for`, `do`-`while`, returns, prefix/postfix increment and
decrement, `cout`/`cin` stream chains, qualified stream names, supported include
lines, and `using namespace std;`.

See [docs/cpp-subset-spec.md](docs/cpp-subset-spec.md) for the authoritative
grammar boundaries. This is **not a complete C or C++ compiler**.

## Build and Test

Dependencies: CMake 3.20+, a C++17 compiler (GCC/MinGW-w64), Flex 2.6+, and
Bison 3.8+. On Windows, use WinFlexBison (`win_flex` / `win_bison`) on `PATH`.

Linux/macOS:

```sh
make cpp         # C/C++ lexer, parser, AST, and CLI
make test-cpp    # C/C++ lexer/parser/AST/CLI tests
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

## Running the C/C++ Frontend

### Token table

```sh
./build/cpp-frontend/optiq_cpp --tokens examples/cpp-subset/01-full.cpp
./build/cpp-frontend/optiq_cpp examples/cpp-subset/01-full.cpp
```

`--tokens` remains the default mode and preserves the Week 2 output format. The
complete sample output is in
[examples/cpp-subset/01-full.tokens.txt](examples/cpp-subset/01-full.tokens.txt).

### Syntax validation

```sh
./build/cpp-frontend/optiq_cpp --parse examples/cpp-subset/03-parser-full.cpp
```

Successful output includes:

```text
Lexer: SUCCESS
Parser: SUCCESS
AST: GENERATED

Syntax analysis completed successfully.
```

### Preliminary AST

```sh
./build/cpp-frontend/optiq_cpp --ast examples/cpp-subset/03-parser-full.cpp
```

The indented tree includes node kinds, operators, identifiers, literal values,
and one-based source locations.

PowerShell equivalents:

```powershell
.\build\cpp-frontend\optiq_cpp.exe --tokens .\examples\cpp-subset\01-full.cpp
.\build\cpp-frontend\optiq_cpp.exe --parse .\examples\cpp-subset\03-parser-full.cpp
.\build\cpp-frontend\optiq_cpp.exe --ast .\examples\cpp-subset\03-parser-full.cpp
Get-Content .\examples\cpp-subset\03-parser-full.cpp -Raw | .\build\cpp-frontend\optiq_cpp.exe --parse
```

C/C++ CLI exit codes are `0` for success, `2` for lexical or syntax errors, and
`1` for command-line, file-I/O, or internal errors. The parser does not run when
lexical analysis fails. Failed syntax analysis never exposes a partial AST.

## Academic Mini-Language Compiler

The mini language supports `int`, `float`, and `bool`; declarations,
assignments, `if`/`else`, `while`, `print`, nested blocks, expressions, comments,
semantic checks, and TAC generation. Its authoritative grammar and semantic
rules remain in [docs/language-spec.md](docs/language-spec.md).

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
│       ├── CMakeLists.txt         # Standalone C++17/Flex/Bison build
│       ├── include/optiq/cpp/     # Tokens, lexer, parser, AST, diagnostics
│       └── src/                   # Flex, Bison, AST printer, adapter, CLI
├── tests/
│   ├── compiler/                  # Existing academic unit/golden tests
│   ├── programs/                  # Eleven academic test programs
│   ├── expected/                  # Academic golden outputs
│   └── cpp-frontend/              # C/C++ lexer, parser, AST, and CLI tests
├── examples/
│   ├── *.mc                       # Six academic examples
│   └── cpp-subset/                # Valid/error samples and token output
├── docs/
│   ├── cpp-subset-spec.md         # Primary C/C++ grammar and limitations
│   ├── architecture.md            # Independent frontend pipelines
│   ├── parser-design.md           # Bison integration and AST design
│   ├── development-roadmap.md     # Completed and future phases
│   ├── verification.md            # Executed build/test records
│   ├── week1-completion.md        # Foundation report
│   ├── week2-completion.md        # Lexer report with every test result
│   ├── week3-completion.md        # Parser report with every test result
│   └── project-report.md          # Original academic report
├── Makefile                       # Independent and combined build targets
└── README.md
```

## Verification

Week 3 verification executed **13/13 academic CTest tests** and **64/64 C/C++
CTest tests**, all passing. The C/C++ total includes 18 lexer categories, 23
positive parser tests, 12 negative parser tests, five structural AST/type/
recovery tests, and six CLI tests. See [docs/verification.md](docs/verification.md)
and [docs/week3-completion.md](docs/week3-completion.md). Windows commands are
provided but are not claimed as executed in that record.

## Branch Preservation

- `academic-baseline` freezes the original academic implementation and history.
- `main` is the primary C/C++ product after the tested migration was approved.
- `sql-prototype` is unrelated to this migration and remains untouched.
- Week 3 publication uses normal commits on `main`; no branch history is
  rewritten and no branch is deleted by the implementation workflow.

## Documentation

- [Architecture](docs/architecture.md)
- [C/C++ subset specification](docs/cpp-subset-spec.md)
- [Parser design](docs/parser-design.md)
- [Development roadmap](docs/development-roadmap.md)
- [Week 1 report](docs/week1-completion.md)
- [Week 2 report](docs/week2-completion.md)
- [Week 3 report](docs/week3-completion.md)
- [Academic project report](docs/project-report.md)

The academic project report describes the original mini-language submission; it
does not claim C/C++ frontend features.

## Out of Scope After Week 3

Complete C or C++, full preprocessing, C/C++ semantic analysis, type checking,
name resolution, TAC, optimization, machine code, linking, and executables.
Parsing success does not imply that a program is semantically valid or runnable.

## License

MIT
