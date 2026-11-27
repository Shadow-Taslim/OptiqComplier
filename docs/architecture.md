# OptiQ Architecture

OptiQ has two deliberately independent C++17 frontends. The primary product is
the C/C++ common-subset compiler; the original academic mini-language compiler
remains independently buildable and testable.

## Frontend Separation

```text
Primary C/C++ source (.cpp)                Academic source (.mc / .txt)
          │                                        │
          ▼                                        ▼
C/C++ Flex lexer                             Mini Flex lexer
compiler/cpp-frontend/                       compiler/src/lexer.*
          │                                        │
          ▼                                        ▼
Located tokens + diagnostics                  Mini Bison parser
          │                                        │
          ▼                                        ▼
C/C++ token adapter                           Mini typed AST
          │                                        │
          ▼                                        ▼
C/C++ Bison parser                     Symbol table + semantic analysis
          │                                        │
          ▼                                        ▼
Preliminary C/C++ AST                           Three Address Code
          │
          ├── AST printer
          └── Future semantics → IR → optimization → backend
```

The two CMake projects produce separate executables, `optiq_cpp` and
`optiq_cli`. No generated Flex/Bison source, grammar symbol, AST, or mutable
scanner state is shared. Only the academic frontend currently performs semantic
analysis and TAC generation.

## Primary C/C++ Frontend

| Module | Files | Responsibility |
| --- | --- | --- |
| Public token model | `include/optiq/cpp/token.hpp` | Explicit token types, lexemes, and one-based source locations. |
| Public diagnostics | `include/optiq/cpp/diagnostics.hpp` | Located lexical and syntax error records. |
| Public lexer API | `include/optiq/cpp/lexer.hpp` | Source string to tokens plus lexical diagnostics. |
| Flex rules | `src/lexer.l` | Longest-match tokens, literals, comments, preprocessor lines, and exclusive states. |
| Scanner state | `src/scanner_state.*` | Reentrant location tracking, keyword classification, literal validation, and diagnostics. |
| Scanner driver | `src/lexer.cpp` | Initializes/destroys Flex safely and returns independent results. |
| Public parser API | `include/optiq/cpp/parser.hpp` | `ParseResult` with owned AST and separate lexical/syntax diagnostics. |
| Bison grammar | `src/parser.y` | Subset CFG, precedence levels, declarations, statements, streams, AST construction, and recovery. |
| Token adapter/parser driver | `src/parser.cpp` | Maps Flex tokens to Bison symbols, supplies EOF locations, and runs parsing. |
| Parser context | `src/parser_context.hpp` | Token cursor, EOF location, AST result, and syntax diagnostics. |
| AST model | `include/optiq/cpp/ast.hpp`, `src/ast.cpp` | Typed extensible nodes, source locations, and `unique_ptr` ownership. |
| AST printer | `include/optiq/cpp/ast_printer.hpp`, `src/ast_printer.cpp` | Stable indented tree with node details and locations. |
| Token names | `src/token.cpp` | Stable uppercase names for token output and tests. |
| CLI | `src/cli.cpp` | `--tokens`, `--parse`, `--ast`, file/stdin input, diagnostics, and exit codes. |

Paths in this table are relative to `compiler/cpp-frontend/`. See
[parser-design.md](parser-design.md) for the integration details.

### C/C++ Design Decisions

- **Reentrant scanner and parser.** Each parse owns its token vector, parser
  context, diagnostics, and AST. No global result data is shared.
- **Unchanged Week 2 scanner.** Bison consumes the existing `LexResult` token
  vector through a small adapter instead of requiring invasive Flex changes.
- **Separate C/C++ AST.** The mini AST encodes another language and is not
  reused. C/C++ nodes use `std::unique_ptr` and preserve source locations.
- **Explicit grammar hierarchy.** Precedence and associativity are encoded by
  expression nonterminals; dangling `else` has explicit precedence.
- **Explicit unsupported behavior.** Invalid numbers, escapes, directives,
  tokens, and unsupported syntax produce diagnostics rather than silent
  reinterpretation.
- **Limited preprocessor nodes.** Supported `#include`/`#define` lines remain
  single tokens and AST leaves; no inclusion or expansion occurs.
- **Failure has no public partial AST.** Recovery helps inspect later statements,
  but `ParseResult::success()` remains false and the result program stays null.

## Academic Mini-Language Frontend

The existing `.mc` pipeline remains source → Flex lexer → Bison parser → AST →
semantic analysis → TAC. Its module responsibilities are unchanged:

| Module | Files | Responsibility |
| --- | --- | --- |
| Lexer | `compiler/src/lexer.l`, `lexer.cpp`, `scanner_state.*` | Character stream to located mini-language tokens. |
| Parser | `compiler/src/parser.y`, `parser.cpp`, `token_stream.hpp` | Verifies the formal CFG, builds the AST, and recovers at statement boundaries. |
| AST | `compiler/include/optiq/ast.hpp`, `compiler/src/ast_printer.cpp` | Typed nodes with ownership and text/JSON printers. |
| Symbol table | `compiler/include/optiq/symbol_table.hpp`, `compiler/src/symbol_table.cpp` | Nested scopes and declaration records. |
| Semantic analyzer | `compiler/include/optiq/semantic.hpp`, `compiler/src/semantic.cpp` | Declaration, scope, operand, assignment, and condition rules. |
| TAC generator | `compiler/include/optiq/tac.hpp`, `compiler/src/tac.cpp` | Temporaries, labels, jumps, control flow, and `print`. |
| CLI | `compiler/src/cli.cpp` | Tokens, AST, JSON, semantic checking, or TAC; file/argument/stdin input. |

The formal grammar and semantic rules remain in
[language-spec.md](language-spec.md), and the academic report remains in
[project-report.md](project-report.md).

## Error Models

| Executable/mode | Exit code | Meaning |
| --- | --- | --- |
| `optiq_cpp` | `0` | Requested tokenization, parsing, or AST printing succeeded |
| `optiq_cpp` | `2` | One or more lexical or syntax diagnostics |
| `optiq_cpp` | `1` | Command-line, file-I/O, or internal error |
| `optiq_cli` | `0` | Mini-language program compiled to TAC |
| `optiq_cli` | `2` | Mini-language lexical or syntax error |
| `optiq_cli` | `3` | Mini-language semantic error |
| `optiq_cli` | `1` | Internal compiler error |

The C/C++ parser is not invoked after lexical failure. Syntax diagnostics use
`source:line:column: syntax error: ...`; token mode retains its Week 2 diagnostic
format.

## Testing Layers

1. **C/C++ lexer categories:** eighteen entries cover every requested lexical
   category, positive and negative behavior, and exact locations.
2. **C/C++ positive parser tests:** twenty-three entries cover the documented
   translation-unit, declaration, expression, control-flow, function, stream,
   include, and namespace syntax.
3. **C/C++ negative parser tests:** twelve entries require rejection and located
   diagnostics for malformed syntax.
4. **C/C++ structural tests:** five entries inspect precedence, assignment
   associativity, AST locations/printing, recovery, and `void` restrictions.
5. **C/C++ CLI acceptance:** six entries cover tokens, lexical errors, parse,
   AST, syntax errors, and help.
6. **Academic tests:** thirteen unit, golden, and CLI entries remain unchanged.

The suites run separately with `make test-cpp` and `make test-mini`, or together
with `make test`.

## Build Architecture

`compiler/CMakeLists.txt` remains the standalone academic project.
`compiler/cpp-frontend/CMakeLists.txt` is the standalone C/C++ project. The root
Makefile supplies convenience targets but does not create a dependency between
either compiler.

CMake generates scanner/parser sources inside the selected build directory. The
verified Week 3 environment is GNU C++ 12.2, Flex 2.6.4, Bison 3.8.2, CMake
3.25.1 on Linux. Windows PowerShell commands use win_flex/win_bison with MinGW
Makefiles; a separate native run is required before claiming Windows
verification.
