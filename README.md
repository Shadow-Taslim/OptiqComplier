# Optiq — Mini Language Compiler (Flex + Bison)

A compiler front-end with **intermediate code generation** for the
course-specified mini language (Compiler Construction Lab, Metropolitan
University). Optiq compiles `.mc` source through the full front-end pipeline —
**lexical analysis (Flex) → syntax analysis (Bison) → AST → semantic analysis
(symbol table + type checking) → Three Address Code (TAC)**.

> **Status:** all six required modules are implemented and tested. The
> language, grammar, and semantic rules follow the project manual exactly
> (see [docs/language-spec.md](docs/language-spec.md)).

## Language at a Glance

```c
int x;
int y;
bool flag;

x = 10;
y = 0;
flag = true;

while (x > 0) {
  y = y + x;
  x = x - 1;
}

if (flag == true) {
  print y;
} else {
  print x;
}
```

Types `int`, `float`, `bool`; statements: declaration, assignment, `if`,
`if-else`, `while`, `print`, nested `{ ... }` blocks with proper scoping;
operators `+ - * / %`, `< > <= >= == !=`, `&& || !`; `//` and `/* */`
comments. Full formal CFG: [docs/language-spec.md](docs/language-spec.md).

## Repository Structure

```text
├── compiler/
│   ├── include/optiq/       # AST, lexer, parser, symbol table, semantic, TAC interfaces
│   ├── src/
│   │   ├── lexer.l          # Flex token rules
│   │   ├── parser.y         # Bison grammar (conflict-free, with error recovery)
│   │   ├── scanner_state.*  # Token construction, locations, keyword table
│   │   ├── lexer.cpp        # Reentrant scanner driver
│   │   ├── parser.cpp       # Token-to-Bison bridge, parse entry point
│   │   ├── ast_printer.cpp  # Indented-text and JSON AST printers
│   │   ├── symbol_table.cpp # Nested-scope symbol table
│   │   ├── semantic.cpp     # Semantic analyzer (type checking)
│   │   ├── tac.cpp          # Three Address Code generator
│   │   └── cli.cpp          # optiq_cli entry point
│   └── CMakeLists.txt
├── examples/                # Six valid demo programs (.mc)
├── tests/
│   ├── compiler/            # Unit tests + golden-file runner
│   ├── programs/            # Test programs: valid + lexical/syntax/semantic errors
│   └── expected/            # Expected output paired with each test program
├── docs/
│   ├── language-spec.md     # Formal language specification (CFG + semantic rules)
│   ├── architecture.md      # Pipeline, module design, decisions
│   └── verification.md      # Build/test verification record
├── Makefile                 # Convenience wrapper around CMake
└── README.md
```

## Dependencies

- CMake 3.20+, a C++17 compiler (GCC/MinGW-w64), Flex 2.6+, Bison 3.0+
  (`win_flex` / `win_bison` on Windows)

## Build and Test

Linux/macOS:

```sh
make            # configure + build into build/compiler
make test       # run the full CTest suite
```

Windows (PowerShell, MinGW + WinFlexBison on `PATH`):

```powershell
cmake -S compiler -B build/compiler -G "MinGW Makefiles" -DOPTIQ_BUILD_TESTS=ON
cmake --build build/compiler --parallel
ctest --test-dir build/compiler --output-on-failure
```

Generated scanner/parser sources live in the build directory and are never
committed.

## Running the Compiler

Compile a program to TAC (the final output):

```sh
./build/compiler/optiq_cli examples/06-manual-sample.mc
```

Other modes:

```sh
./build/compiler/optiq_cli --tokens examples/01-variables.mc   # token stream
./build/compiler/optiq_cli --ast    examples/03-if-else.mc     # indented AST
./build/compiler/optiq_cli --json   examples/03-if-else.mc     # AST as JSON
./build/compiler/optiq_cli --check  examples/04-while-loop.mc  # semantics only
Get-Content .\examples\06-manual-sample.mc -Raw | .\build\compiler\optiq_cli.exe  # stdin (PowerShell)
```

Exit codes: `0` compiled · `2` lexical/syntax error · `3` semantic error ·
`1` internal error.

### Example output

`examples/02-arithmetic.mc` compiles to:

```text
a = 5
b = 10
t1 = b * 2
t2 = a + t1
result = t2
print result
t3 = a + b
t4 = t3 * 2
print t4
t5 = result % 4
print t5
```

and `tests/programs/05-error-semantic-undeclared.mc` is rejected with:

```text
Semantic error at line 3, column 1: Undeclared variable 'total' (not visible in this scope)
```

## Verification

The full suite — 13 CTest tests covering the lexer, parser, AST, semantic
analyzer, TAC generator, golden-file program tests (all 11 test programs
paired with expected output), and the CLI — passes with **0 grammar
conflicts** and **0 compiler warnings**. Details:
[docs/verification.md](docs/verification.md).

## Out of Scope (per the project manual)

Machine code, assembly, register allocation, linking, executables, and
optimization are intentionally not part of this project.

## License

MIT
