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
Located C/C++ tokens                         Mini Bison parser
and diagnostics                                        │
          │                                            ▼
          │                                    Mini typed AST
          │                                            │
          │                                            ▼
          │                              Symbol table + semantic analysis
          │                                            │
          │                                            ▼
          │                                    Three Address Code
          ▼
Future C/C++ parser → AST → semantics → IR → optimization → backend
(only the C/C++ lexer is implemented in Weeks 1–2)
```

The two CMake projects produce separate executables, `optiq_cpp` and
`optiq_cli`. No generated Flex/Bison source or global scanner state is shared.

## Primary C/C++ Frontend

| Module | Files | Responsibility |
| --- | --- | --- |
| Public token model | `compiler/cpp-frontend/include/optiq/cpp/token.hpp` | Explicit token types, lexemes, and one-based source locations. |
| Public diagnostics | `compiler/cpp-frontend/include/optiq/cpp/diagnostics.hpp` | Located lexical error records. |
| Public lexer API | `compiler/cpp-frontend/include/optiq/cpp/lexer.hpp` | Source string to tokens plus all diagnostics; `success()` reports lexical validity. |
| Flex rules | `compiler/cpp-frontend/src/lexer.l` | Longest-match tokens, literals, comments, preprocessor lines, and exclusive lexer states. |
| Scanner state | `compiler/cpp-frontend/src/scanner_state.*` | Reentrant location tracking, keyword classification, literal validation, and diagnostic collection. |
| Scanner driver | `compiler/cpp-frontend/src/lexer.cpp` | Initializes/destroys Flex safely, scans byte buffers, and returns an independent result. |
| Token names | `compiler/cpp-frontend/src/token.cpp` | Stable uppercase names for CLI output and tests. |
| CLI | `compiler/cpp-frontend/src/cli.cpp` | File or stdin input, token table, diagnostics, help, and exit codes 0/2/1. |

### C/C++ Design Decisions

- **Reentrant Flex scanner.** Mutable scanning state belongs to one
  `ScannerState`; concurrent or repeated tokenization does not rely on mutable
  global result data.
- **Complete tokenization.** The public API returns all valid tokens and all
  collected diagnostics. Invalid characters do not hide later errors.
- **Exclusive literal/comment states.** Strings, character literals, and block
  comments cannot be confused with operators or comment markers in their text.
- **Explicit unsupported behavior.** Invalid numbers, escapes, directives, and
  unterminated constructs produce diagnostics instead of silent reinterpretation.
- **Limited preprocessor tokens only.** Supported `#include`/`#define` lines
  remain single tokens; no inclusion or macro expansion occurs.
- **No premature AST reuse.** The mini AST encodes another language. A C/C++ AST
  will be designed with its grammar in a later phase.

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

| Executable | Exit code | Meaning |
| --- | --- | --- |
| `optiq_cpp` | `0` | C/C++ subset tokenized without lexical diagnostics |
| `optiq_cpp` | `2` | One or more lexical diagnostics |
| `optiq_cpp` | `1` | Command-line, file-I/O, or internal error |
| `optiq_cli` | `0` | Mini-language program compiled to TAC |
| `optiq_cli` | `2` | Mini-language lexical or syntax error |
| `optiq_cli` | `3` | Mini-language semantic error |
| `optiq_cli` | `1` | Internal compiler error |

## Testing Layers

1. **C/C++ lexer categories:** eighteen CTest entries cover every requested
   lexical category, positive and negative behavior, and exact locations.
2. **C/C++ CLI acceptance:** full sample, invalid sample, and help entries.
3. **Academic unit tests:** lexer, parser, AST, semantic analyzer, and TAC.
4. **Academic golden files:** eleven programs compared with expected output.
5. **Academic CLI tests:** file, argument, stdin, help, and failure modes.

The suites run separately with `make test-cpp` and `make test-mini`, or together
with `make test`.

## Build Architecture

`compiler/CMakeLists.txt` remains the standalone academic project.
`compiler/cpp-frontend/CMakeLists.txt` is the standalone C/C++ project. The
root Makefile supplies convenience targets but does not create a dependency
between either compiler.

CMake generates scanner/parser sources inside the selected build directory.
The verified migration environment is GNU C++ 12.2, Flex 2.6.4, Bison 3.8.2,
CMake 3.25.1 on Linux. Windows PowerShell commands use win_flex/win_bison with
MinGW Makefiles; a separate native run is required before claiming Windows
verification.
