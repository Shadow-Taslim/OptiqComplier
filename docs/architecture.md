# Architecture and Implementation Plan

Optiq is a compiler front-end with intermediate code generation for the
course-specified language (see [language-spec.md](language-spec.md)). It is a
single C++17 pipeline: source code in, Three Address Code out.

## Pipeline

```text
Source code (.mc / .txt)
        │
        ▼
Lexical Analyzer (Flex)          compiler/src/lexer.l, lexer.cpp, scanner_state.cpp
        │  token stream with line/column locations
        ▼
Syntax Analyzer (Bison)          compiler/src/parser.y, parser.cpp
        │  Abstract Syntax Tree (AST)
        ▼
Semantic Analyzer                compiler/src/semantic.cpp
(Symbol Table + Type Checking)   compiler/src/symbol_table.cpp
        │  annotated / validated AST
        ▼
Intermediate Code Generation     compiler/src/tac.cpp
        │
        ▼
Three Address Code (TAC) output
```

## Module Responsibilities

| Module | Files | Responsibility |
| ------ | ----- | -------------- |
| Lexer | `src/lexer.l`, `src/lexer.cpp`, `src/scanner_state.*` | Character stream → token stream; discards comments/whitespace; reports invalid tokens with line and column. |
| Parser | `src/parser.y`, `src/parser.cpp`, `src/token_stream.hpp` | Verifies the CFG; builds the AST; reports syntax errors with locations and recovers at statement boundaries (`error SEMICOLON`) so one error does not hide the rest. |
| AST | `include/optiq/ast.hpp`, `src/ast_printer.cpp` | Typed node hierarchy (`Declaration`, `Assignment`, `If`, `While`, `Print`, `Block`; identifier/literal/binary/unary expressions) with `std::unique_ptr` ownership; indented-text and JSON printers. |
| Symbol table | `include/optiq/symbol_table.hpp`, `src/symbol_table.cpp` | Scope stack; each entry records name, type, scope id, and declaration line. |
| Semantic analyzer | `include/optiq/semantic.hpp`, `src/semantic.cpp` | AST walk enforcing declaration-before-use, redeclaration, scope visibility, and the full type system. |
| TAC generator | `include/optiq/tac.hpp`, `src/tac.cpp` | AST walk emitting temporaries, labels, conditional/unconditional jumps, and `print` instructions. |
| CLI | `src/cli.cpp` | `--tokens`, `--ast`, `--json`, `--check`, or default TAC output; file, argument, or stdin input; exit codes 0/2/3 for success, syntax, and semantic failure. |

## Design Decisions

- **Tokenize fully before parsing.** The lexer produces a complete token
  vector; a `TokenStream` feeds Bison one token at a time. This keeps the
  reentrant Flex scanner independent of the parser and makes both sides easy
  to unit-test.
- **Pure, location-aware Bison parser** (`api.pure full`, `%locations`,
  `parse.error verbose`) so diagnostics carry line/column information.
- **Statement-level error recovery** via `statement: error SEMICOLON` with
  `yyerrok`, so a single syntax error does not halt compilation and every
  recoverable error is reported in one run.
- **Semantic analysis is a separate pass**, not embedded parser actions — the
  AST is the single interface between phases, exactly as the manual's pipeline
  requires.
- **Blocks are the only scope boundaries**, so nested-scope handling in the
  symbol table is a simple push/pop around `visitBlock`.
- **Keywords are case-sensitive** (C-family convention), matched by a keyword
  table after the generic identifier pattern is recognized by Flex.

## Error Model

| Exit code | Meaning |
| --------- | ------- |
| `0` | Compiled successfully (TAC emitted) |
| `2` | Lexical or syntax error |
| `3` | Semantic error |
| `1` | Internal compiler error |

## Testing Layers

1. **Unit tests** per module: `lexer_tests`, `parser_tests`, `ast_tests`,
   `semantic_tests`, `tac_tests` (CTest).
2. **Golden-file tests**: every program in `tests/programs/` is compiled and
   compared byte-for-byte with `tests/expected/<name>.out`
   (`example_tests`), covering valid compilation, lexical errors, syntax
   errors, and every semantic rule.
3. **CLI tests**: argument, file, stdin, and help modes, plus invalid-input
   exit codes.

## Build

CMake generates the Flex/Bison sources into the build directory (generated
files are never committed). A root `Makefile` wraps the CMake invocations for
convenience. The verified toolchain is GCC 12.2, Flex 2.6.4, Bison 3.8.2 on
Linux; on Windows use win_flex/win_bison with the MinGW Makefiles generator.
