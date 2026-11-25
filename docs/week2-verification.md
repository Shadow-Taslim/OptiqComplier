# Week 2 Verification

**Date:** 14 November 2026  
**Milestone:** Compiler frontend  
**Result:** **PASS — clean configure/build and 8/8 automated tests**

## Implemented Features

- Reentrant Flex lexer with case-insensitive SQL keywords and source-position tracking.
- Identifiers, qualified-name components, integer/decimal/string literals, operators, punctuation, whitespace, and comments.
- Conflict-free Bison parser for the documented `SELECT` subset.
- Correct arithmetic, comparison, boolean, `NOT`, and unary-expression precedence.
- Strongly typed AST with safe ownership and deterministic JSON printing.
- CLI argument and stdin modes, stdout/stderr separation, and documented exit codes.
- Automated AST, lexer, parser, fixture, and CLI tests.

## Files Created

### Compiler interfaces

- `compiler/include/optiq/ast.hpp`
- `compiler/include/optiq/ast_printer.hpp`
- `compiler/include/optiq/lexer.hpp`
- `compiler/include/optiq/parser.hpp`

### Compiler implementation

- `compiler/src/ast_printer.cpp`
- `compiler/src/cli.cpp`
- `compiler/src/lexer.cpp`
- `compiler/src/lexer.l`
- `compiler/src/parser.cpp`
- `compiler/src/parser.y`
- `compiler/src/scanner_state.cpp`
- `compiler/src/scanner_state.hpp`
- `compiler/src/token_stream.hpp`

### Automated tests

- `tests/compiler/ast_tests.cpp`
- `tests/compiler/lexer_tests.cpp`
- `tests/compiler/parser_tests.cpp`
- `tests/compiler/fixture_tests.cpp`
- `tests/compiler/cli_stdin_test.cmake`
- `tests/fixtures/01-ordered-projection.sql` through `tests/fixtures/12-department-aggregate.sql`

## Files Modified

- `compiler/CMakeLists.txt`
- `README.md`
- `docs/architecture.md`

The existing SQL documentation, JSON contract, sample schemas, CSV data, and JavaScript workspace files were preserved.

## Actual Build Verification

Environment: GNU C++ 12.2.0, Flex 2.6.4, Bison 3.8.2, CMake 3.25.1, Linux.

```sh
cmake -S compiler -B build/compiler -DOPTIQ_BUILD_TESTS=ON
cmake --build build/compiler --parallel
```

**Actual result:** configuration and compilation succeeded. Flex and Bison generated their sources in the build directory. Bison reported **0 grammar conflicts**.

## Actual Test Results

```sh
ctest --test-dir build/compiler --output-on-failure
```

| Test | Result |
| --- | --- |
| `ast_tests` | Passed |
| `lexer_tests` | Passed |
| `parser_tests` | Passed |
| `fixture_tests` — all 12 SQL fixtures | Passed |
| `cli_valid` | Passed |
| `cli_invalid` | Passed |
| `cli_help` | Passed |
| `cli_stdin` | Passed |

**Actual result:** `100% tests passed, 0 tests failed out of 8`.

A direct standard-input run also produced a `SelectStatement` AST, and a direct invalid-query run returned exit code `2` with a line/column diagnostic.

## Known Limitations

- Only the documented `SELECT` grammar subset is accepted.
- A query currently supports one base table and at most one inner join.
- Semantic checks do not yet validate catalog names, ambiguous columns, or types.
- The AST retains constant arithmetic; constant folding is not performed.
- No relational plan, optimization pass, CSV execution, or result rows are produced.
- CLI AST JSON is not the future Express compiler request/response envelope.
- Windows/MinGW instructions are provided, but the verified build recorded here used GNU C++ on Linux.

## Week 3 Dependencies

1. Load and validate the `sample-v1` schemas and CSV catalog.
2. Add semantic analysis for tables, columns, aliases, aggregates, and expression types.
3. Translate the typed AST into the initial relational-algebra plan.
4. Add predicate pushdown, projection pruning, and constant folding with focused tests.
5. Implement CSV execution and compare all 12 query results with `docs/example-queries.md`.
6. Only after the core works, adapt the process boundary to the documented Express JSON contract.

---

# Week 2 Re-Verification — Extended Optiq Language

**Date:** 26 November 2026
**Milestone:** Mini programming language frontend (imperative statements + SQL subset)
**Result:** **PASS — clean configure/build, conflict-free grammar, and 12/12 automated tests**

## What Changed Since the SQL-Only Frontend

- The grammar now parses full Optiq **programs**: `let`, assignment, `print`, `if`/`else if`/`else`, `while`, brace blocks, boolean literals, standalone query statements, and query expressions as the right-hand side of declarations and assignments, per [language-spec.md](language-spec.md).
- New AST nodes: `Program`, the `Statement` hierarchy (`LetStatement`, `AssignStatement`, `PrintStatement`, `IfStatement`, `WhileStatement`, `QueryStatement`), `QueryExpression`, and boolean literals.
- The lexer adds the language keywords, `{` `}`, and `//` line comments.
- The CLI defaults to program mode and accepts `.oq` file paths; `--sql` keeps single-query behavior.

## Design Decisions Recorded

- **Query expressions are restricted** to `let`/assignment right-hand sides and standalone statements. Allowing a bare `SELECT` anywhere inside an expression made the `WHERE`-clause expression ambiguous with enclosing operators (27 shift/reduce conflicts); the restriction keeps the grammar **LALR conflict-free** with `--warnings=all`.
- **Query statements require a semicolon inside programs.** This removes the last shift/reduce conflict (a table alias followed by a new statement starting with an identifier). `parseSql` appends a trailing `;` when missing, so the legacy semicolon-optional single-query behavior is preserved.
- **Keywords cannot be identifiers** (`count`, `select`, `let`, ... are always keywords), matching SQL convention.

## Files Created

- `docs/language-spec.md`, `docs/worked-example.md`
- `examples/01-variables-and-print.oq` through `examples/10-full-pipeline.oq`
- `tests/compiler/program_tests.cpp`
- `tests/compiler/program_fixture_tests.cpp`

## Files Modified

- `compiler/include/optiq/ast.hpp` — statement hierarchy, `Program`, `QueryExpression`, boolean literals
- `compiler/include/optiq/ast_printer.hpp`, `compiler/src/ast_printer.cpp` — program/statement JSON printing
- `compiler/include/optiq/lexer.hpp`, `compiler/src/lexer.cpp`, `compiler/src/lexer.l`, `compiler/src/scanner_state.cpp` — language tokens and comments
- `compiler/include/optiq/parser.hpp`, `compiler/src/parser.y`, `compiler/src/parser.cpp` — program grammar, `parseProgram`, `parseSql` compatibility wrapper
- `compiler/src/cli.cpp` — file input, program/`--sql` modes
- `compiler/CMakeLists.txt` — new test targets
- `tests/compiler/ast_tests.cpp`, `tests/compiler/lexer_tests.cpp` — extended coverage
- `README.md`, `docs/architecture.md`, `docs/api-contract.md`

## Actual Build Verification

Environment: GNU C++ 12.2.0, Flex 2.6.4, Bison 3.8.2, CMake 4.4.5, Linux.

```sh
cmake -S compiler -B build/compiler -DOPTIQ_BUILD_TESTS=ON
cmake --build build/compiler --parallel
```

**Actual result:** configuration and compilation succeeded with **zero compiler warnings**; Bison reported **0 grammar conflicts**.

## Actual Test Results

```sh
ctest --test-dir build/compiler --output-on-failure
```

| Test | Result |
| --- | --- |
| `ast_tests` (now including program AST printing) | Passed |
| `lexer_tests` (now including language keywords, braces, `//` comments) | Passed |
| `parser_tests` (SQL grammar, via the `parseSql` wrapper) | Passed |
| `fixture_tests` — all 12 SQL fixtures | Passed |
| `program_tests` — statements, query expressions, precedence, 10 invalid-program rejections | Passed |
| `program_fixture_tests` — all 10 Optiq example programs | Passed |
| `cli_valid` | Passed |
| `cli_program` | Passed |
| `cli_invalid` | Passed |
| `cli_invalid_program` | Passed |
| `cli_help` | Passed |
| `cli_stdin` | Passed |

**Actual result:** `100% tests passed, 0 tests failed out of 12`.

A direct CLI run on `examples/02-assignment-and-loop.oq` printed a `Program` AST containing `LetStatement` and `WhileStatement` nodes; `let x = 5` (missing semicolon) exited with code `2` and a line/column diagnostic.

## Known Limitations (Extended Scope)

- Programs parse into an AST but are **not interpreted**; `print` produces no runtime output yet.
- Name resolution (variable vs column) and type checking are Week 3 semantic work.
- Query expressions cannot nest inside arbitrary expressions by design (see above).
- All limitations from the SQL-only frontend above still apply to the embedded queries.
