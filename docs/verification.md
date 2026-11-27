# Verification Record

## C/C++ Migration Verification

**Date:** 27 November 2026  
**Milestone:** Week 1 architecture + Week 2 C/C++ lexer  
**Result:** **PASS — 13/13 academic tests and 21/21 C/C++ tests**  
**Verified GitHub branch:** `cpp-migration`  
**Verified GitHub revision:** `ecc360712c8d65c5958e362ccead5792b9eca76a`  
**Academic base:** `academic-baseline` commit
`5179118fbf40a5a9a50a90d31f02651c6e73b7e4`.

The complete branch was downloaded fresh from GitHub after publishing. Both
standalone builds and both complete test suites were executed against that
remote archive.

Environment: GNU C++ 12.2.0, Flex 2.6.4, Bison 3.8.2, CMake 3.25.1, Linux.

### Executed commands

```sh
cmake -S compiler -B build/compiler -DOPTIQ_BUILD_TESTS=ON
cmake --build build/compiler --parallel
ctest --test-dir build/compiler --output-on-failure

cmake -S compiler/cpp-frontend -B build/cpp-frontend -DOPTIQ_CPP_BUILD_TESTS=ON
cmake --build build/cpp-frontend --parallel
ctest --test-dir build/cpp-frontend --output-on-failure
```

### Actual results

| Suite | Expected | Actual | Result |
| --- | --- | --- | --- |
| Academic configure/build | Existing compiler remains independently buildable | Standalone configure/build completed | PASS |
| Academic CTest | Thirteen existing tests pass | `100% tests passed, 0 tests failed out of 13` | PASS |
| C/C++ configure/build | New frontend independently builds with C++17/Flex | Standalone configure/build completed | PASS |
| C/C++ CTest | Eighteen lexer categories plus three CLI tests pass | `100% tests passed, 0 tests failed out of 21` | PASS |
| C/C++ valid sample | Full sample tokenizes with exit 0 | 48 tokens written to `examples/cpp-subset/01-full.tokens.txt`; exit 0 | PASS |
| C/C++ invalid sample | Located diagnostics and exit 2 | Four diagnostics for malformed number, invalid escape, invalid character, unterminated comment; exit 2 | PASS |

No compiler-warning or Bison-conflict output was observed in these executed
builds. No native Windows build was executed; PowerShell instructions are
provided but remain unverified here. Every C/C++ test's input, expected result,
actual result, and status is listed in [week2-completion.md](week2-completion.md).

## Historical Academic Verification

**Date:** 26 November 2026  
**Milestone:** Complete compiler front-end + TAC for the course-specified language
**Result:** **PASS — clean configure/build, conflict-free grammar, 13/13 automated tests**  
**Verified GitHub revision:** `3525ef2948af4ca7f0fb89209c0bbb4510efa9ac` (`main`)

The tracked compiler sources in a freshly downloaded GitHub archive were compared
with the locally assembled tree before this clean verification. The archive did
not contain generated scanner/parser sources or obsolete SQL fixtures.

## Scope Verified

- Flex lexer: keywords (`int float bool if else while print true false`),
  identifiers, integer/float/boolean literals, all arithmetic, relational, and
  logical operators, delimiters, `//` and `/* */` comments, whitespace
  discarding, and line/column-located lexical errors.
- Bison parser: the full CFG of [language-spec.md](language-spec.md), correct
  precedence/associativity, statement-level error recovery (`error SEMICOLON`
  with `yyerrok`), and verbose multi-error diagnostics.
- AST: typed nodes for all statements and expressions; indented-text and JSON
  printers.
- Symbol table: nested scopes with shadowing; entries record name, type,
  scope, and declaration line.
- Semantic analyzer: undeclared use, redeclaration, scope violation,
  declaration/assignment type compatibility, operand rules for
  arithmetic/relational/logical operators, boolean conditions.
- TAC generator: temporaries, labels, `ifFalse`/`goto` control flow for
  `if`/`if-else`/`while`, and `print`.

## Actual Build Verification

Environment: GNU C++ 12.2.0, Flex 2.6.4, Bison 3.8.2, CMake 4.4.5, Linux.

```sh
cmake -S compiler -B build/compiler -DOPTIQ_BUILD_TESTS=ON
cmake --build build/compiler --parallel
```

**Actual result:** configuration and compilation succeeded with **zero
compiler warnings**; Bison reported **0 grammar conflicts** with
`--warnings=all`.

## Actual Test Results

```sh
ctest --test-dir build/compiler --output-on-failure
```

| Test | Result |
| --- | --- |
| `ast_tests` — AST ownership, text/JSON printing | Passed |
| `lexer_tests` — keywords, literals, operators, comments, locations, errors | Passed |
| `parser_tests` — statements, precedence ladder, blocks, error recovery | Passed |
| `semantic_tests` — scoping and type rules, one case per manual §4.5 rule | Passed |
| `tac_tests` — expressions, control flow, print, nested labels | Passed |
| `example_tests` — all 11 `tests/programs/*.mc` vs golden outputs | Passed |
| `cli_valid` / `cli_tokens` / `cli_ast` | Passed |
| `cli_invalid_syntax` / `cli_invalid_semantic` | Passed |
| `cli_help` / `cli_stdin` | Passed |

**Actual result:** `100% tests passed, 0 tests failed out of 13`.

## Known Limitations

- TAC is the final output; there is no interpreter or backend (out of scope
  per the manual).
- Golden files for syntax-error wording were captured with Bison 3.8.2; other
  Bison versions may phrase `syntax error, unexpected ...` messages slightly
  differently.
- Error recovery resynchronizes at semicolons; a missing closing brace is
  reported once at end of input.
- Bonus features (for/do-while/switch, `++`/`--`, constant folding, Graphviz)
  are not implemented yet.
