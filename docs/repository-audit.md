# Repository Audit

**Audit base:** `main` / `academic-baseline` at
`5179118fbf40a5a9a50a90d31f02651c6e73b7e4`.

## Existing implementation

The repository contains a complete C++17 front-end and TAC generator for the
academic OptiQ mini language. Its `.mc` pipeline is Flex lexical analysis,
Bison syntax analysis, typed AST construction, nested-scope symbol table and
semantic analysis, then Three Address Code generation.

The existing implementation includes:

- Reentrant Flex scanner with token locations and lexical diagnostics.
- Location-aware Bison grammar with precedence, associativity, verbose errors,
  and statement-level recovery.
- AST ownership through `std::unique_ptr`, plus text and JSON printers.
- Symbol-table scoping, declaration/use checks, and type rules.
- TAC expressions, temporaries, labels, jumps, `if`, `while`, and `print`.
- `optiq_cli` file, argument, and standard-input modes.
- CMake 3.20 build, a root Makefile wrapper, thirteen CTest entries, eleven
  golden-file programs, six examples, a formal language specification, and an
  academic project report.

## Reusable components

The migration reuses architectural patterns rather than coupling the two
languages:

- Reentrant Flex setup and isolated scanner state.
- One-based source-location tracking.
- Token-vector output and independent lexer unit tests.
- CMake-generated scanner sources and CTest organization.
- CLI file/stdin patterns and diagnostic formatting.
- Golden-file and example-driven regression testing.

The mini-language AST, symbol table, semantic analyzer, and TAC generator remain
reusable by the academic compiler, but are not imposed on the C/C++ frontend.
Their types and grammar encode mini-language rules that do not directly match
C/C++.

## Gaps in the original repository

- No C/C++ token definitions, lexer, CLI, examples, specification, or tests.
- No root CMake project; the existing standalone project is rooted at
  `compiler/`.
- Generic `optiq_*` target names describe only the mini-language pipeline.
- The original Makefile has no independent mini/C++ build entry points and uses
  the non-portable `rm -rf` clean command.

## Compatibility risks and decisions

| Risk | Decision |
| --- | --- |
| Moving the academic compiler could break includes, tests, and report commands. | Retain all existing paths and its standalone `compiler/CMakeLists.txt`. |
| Flex scanners and future Bison parsers may export colliding symbols. | Use separate libraries and executables: `optiq_cpp` and `optiq_cli`. |
| The mini AST and token kinds do not describe the C/C++ subset. | Add the isolated `optiq::cpp` namespace and explicit C/C++ token types. |
| A combined build could make one frontend's failure block the other. | Provide independent CMake projects and independent test commands. |
| Tokenization could be misrepresented as full C/C++ compilation. | State lexer-only status in the README, specification, architecture, CLI help, and reports. |
| The older `sql-prototype` branch contains unrelated API/web history. | Leave that branch unchanged and exclude it from the migration. |
| Windows claims require native verification. | Document PowerShell commands, but report only the Linux environment actually executed. |

## Baseline verification performed during migration

The untouched academic source was configured, built, and tested before frontend
changes using GNU C++ 12.2.0, Flex 2.6.4, Bison 3.8.2, CMake 3.25.1 on Linux.
All **13/13 CTest tests passed**. The subsequent combined verification is in
[verification.md](verification.md).

## Branch audit

- `academic-baseline` preserves the exact original `main` snapshot.
- `main` remains unchanged until the migration is tested and approved.
- `sql-prototype` remains unchanged.
- Local implementation and testing do not constitute a commit, push, or merge.
