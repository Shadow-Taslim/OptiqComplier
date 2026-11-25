# Architecture and Implementation Plan

## Status

Week 1 established the repository layout, interface contract, sample catalog, and acceptance expectations, and was then extended with the [Optiq Language Specification](language-spec.md): Optiq is a **mini programming language** with first-class SQL queries. Week 2 implements the compiler frontend: Flex lexical analysis, Bison parsing for both the imperative language (variables, assignment, `if`/`else`, `while`, `print`) and the SQL subset, a strongly typed C++17 AST, JSON AST printing, source-located diagnostics, the `optiq_cli` executable, and automated tests.

Semantic checks, relational planning, optimization, CSV execution, interpretation, Express handlers, and the React interface remain intentionally unimplemented.

## Components

1. **AST and printer (`compiler/include/optiq/`, `compiler/src/ast_printer.cpp`)** — Typed program, statement, clause, and expression nodes with explicit ownership and deterministic JSON traversal. The `Statement` hierarchy covers `let`, assignment, `print`, `if`/`else`, `while`, and query statements; a `Program` owns the top-level statement list.
2. **Lexer (`compiler/src/lexer.l`)** — Reentrant Flex scanner that produces a standalone token stream. `ScannerState` owns tokenization results and tracks line, column, and byte-offset positions.
3. **Parser (`compiler/src/parser.y`)** — Pure Bison parser that consumes the token stream and constructs the AST. Lexer and parser errors use the same `Diagnostic` shape.
4. **CLI (`compiler/src/cli.cpp`)** — Accepts an Optiq program from a `.oq` file, arguments, or stdin (plus a `--sql` single-query mode), writes AST JSON to stdout on success, and writes diagnostics to stderr on failure.
5. **Express API (`apps/api/`)** — Planned request validation, compiler process management, timeouts, and safe error propagation.
6. **React frontend (`apps/web/`)** — Planned SQL input, compile/execute actions, plan/result views, errors, and statistics.
7. **Sample catalog (`data/`)** — Versioned table metadata and small CSV datasets for later semantic and execution milestones.
8. **Tests (`tests/`)** — AST, lexer, parser, CLI, and 12-fixture acceptance tests; API, optimizer, executor, and frontend tests will accompany those implementations.

## Compiler Frontend Flow

```text
Optiq source (program or single SQL query)
   │
   ▼
Flex scanner + ScannerState
   │ vector<Token>
   │ each token: kind, original lexeme, typed value, source location
   ▼
TokenStream
   │ yylex adapter maps TokenKind to Bison tokens
   ▼
Bison LALR parser
   │ unique_ptr<Statement> / unique_ptr<Expression> and typed clause values
   ▼
Program AST (statements; query statements wrap a SelectStatement)
   │
   ├── success ──> AstPrinter ──> JSON stdout
   └── failure ──> Diagnostic ──> line/column stderr
```

The lexer is deliberately independent of generated Bison headers. The parser adapts its token stream in `parser.cpp`, preserving clear lexer/parser separation and allowing lexical tests to run without invoking the grammar.

## Optiq Language Grammar Boundary

The extended Week 2 frontend accepts full Optiq programs as defined by the EBNF in [language-spec.md](language-spec.md):

- `LET` declarations, reassignment, and `PRINT` output, each terminated by `;`.
- `IF` / `ELSE IF` / `ELSE` conditionals and `WHILE` loops with brace-delimited blocks; blocks nest arbitrarily.
- Boolean literals `TRUE`/`FALSE` and the same expression precedence ladder used by SQL.
- A standalone `SELECT ... ;` query statement, and query expressions such as `let rows = SELECT ...;`.
- `//`, `--`, and `/* ... */` comments.

Every SQL query from the previous SQL-only frontend remains a valid Optiq program: it parses as a one-statement `Program`. `parseSql` is retained as a compatibility entry point that accepts exactly one query and rejects anything else.

## SQL Grammar Boundary

The implemented frontend accepts the 12 documented fixtures and the following Week 2 grammar:

- One `SELECT` statement; as a query statement inside a program it ends with `;`.
- Wildcard and expression select lists.
- One base table with an optional alias, with or without `AS`.
- Zero or one `JOIN`/`INNER JOIN` with an `ON` expression.
- Optional `WHERE`, `GROUP BY`, `ORDER BY`, and `LIMIT` clauses in SQL order.
- Qualified and unqualified column references.
- Integer, decimal, and single-quoted string literals; doubled single quotes escape a quote.
- `COUNT`, `SUM`, `AVG`, `MIN`, and `MAX`, including `COUNT(*)`.
- Comparison, arithmetic, boolean, unary-sign, and parenthesized expressions.

No other statement types are accepted. Syntax outside this boundary produces a lexical or syntax error rather than being silently ignored.

## Operator Precedence

The grammar uses separate nonterminal levels instead of conflict-suppression declarations. From lowest to highest precedence:

1. `OR`
2. `AND`
3. Prefix `NOT`
4. Comparisons: `=`, `==`, `!=`, `<>`, `<`, `<=`, `>`, `>=`
5. Addition and subtraction
6. Multiplication and division
7. Unary `+` and `-`
8. Columns, literals, aggregate calls, and parenthesized expressions

Arithmetic, boolean, and comparison levels are left associative where repeated operators are permitted. The grammar builds with Bison warnings enabled and no shift/reduce or reduce/reduce conflicts.

## AST Ownership and Traversal

`Expression` is the abstract base for `ColumnReference`, `LiteralExpression`, `BinaryExpression`, `UnaryExpression`, `FunctionCall`, and `QueryExpression`; `Statement` is the abstract base for the six statement nodes, and a `Program` owns the top-level statements. Statement and clause structures use value semantics where practical and `std::unique_ptr` for owned children. Bison semantic values use raw pointers only during reduction; each successful reduction transfers ownership into AST nodes, and `%destructor` rules release discarded semantic values on parser failure.

`AstPrinter` traverses nodes through their typed `Expression::Kind` and `Statement::Kind`, producing deterministic JSON without adding a third-party JSON dependency. AST JSON is a Week 2 diagnostic representation and is distinct from the future compile/execute API envelope.

## Source Locations and Diagnostics

Flex updates line, column, and offset positions for every matched token, including whitespace and comments. Lexical failures identify invalid characters, malformed or out-of-range numbers, unterminated strings, and unterminated block comments. Bison verbose syntax errors are annotated with the unexpected token's line and column. The CLI returns `2` for these user-input errors and reserves `1` for unexpected internal failures.

## Build Organization

CMake builds three libraries and one executable:

- `optiq_ast` — AST printing and traversal.
- `optiq_lexer` — Flex scanner and tokenization API.
- `optiq_parser` — Bison grammar and token-stream adapter.
- `optiq_cli` — command-line entry point.

`FLEX_TARGET` and `BISON_TARGET` generate `sql_lexer.cpp`, `sql_parser.cpp`, and `sql_parser.hpp` under the build directory. Generated files are not committed. `OPTIQ_BUILD_TESTS=ON` enables the CTest suite.

## Future Application Request Flow

```text
React SQL editor
    │ HTTP POST /api/compile or /api/execute
    ▼
Express validation and process adapter
    │ one versioned JSON document on stdin
    ▼
C++ compiler process
    ├── lex / parse / semantic analysis
    ├── initial relational plan
    ├── optimized relational plan
    └── execute against sample-v1 CSV data (execute only)
    │ one JSON response on stdout; diagnostics on stderr
    ▼
Express response
    │
    ▼
React plans, errors, results, and statistics views
```

The browser will never start the compiler directly. Express will not pass arbitrary filesystem paths from the browser; the catalog identifier will be allow-listed. Message shapes and failure rules remain defined in [api-contract.md](api-contract.md).

## Test Layers

- `ast_tests` — typed AST construction, ownership-facing behavior, traversal, aliases, wildcards, and JSON escaping.
- `lexer_tests` — keywords, token categories, literals, comments, source locations, and lexical failures.
- `parser_tests` — grammar coverage, precedence, AST structure, empty/missing clauses, invalid characters, and diagnostics.
- `fixture_tests` — all 12 documented SQL fixtures and their expected AST structures.
- `program_tests` — statement grammar, query expressions, program-level precedence, and invalid-program diagnostics.
- `program_fixture_tests` — all 10 documented Optiq example programs and their expected statement structures.
- `cli_valid`, `cli_program`, `cli_invalid`, `cli_invalid_program`, `cli_help`, and `cli_stdin` — output, exit behavior, help, argument, program, and standard-input modes.

Fixture parsing does not execute the query or compare result rows. Execution tests will be added only with the CSV executor.

## Later Implementation Order

1. Add catalog loading, semantic checks, and semantic-error tests (name resolution for variables vs columns).
2. Add the tree-walking interpreter for statements (`let`, assignment, `print`, `if`, `while`).
3. Add relational planning and the three optimization passes.
4. Add CSV execution and verify all documented query results.
5. Adapt the CLI/process boundary to the versioned API contract.
6. Add the Express adapter and API integration tests.
7. Add the React workflow and end-to-end tests.

## Explicit Non-Goals for Week 2

- No program interpretation or execution — statements parse into an AST but produce no runtime behavior yet.
- No semantic validation, relational plans, optimization, or CSV execution.
- No Express routes, React pages, or UI components.
- No database, authentication, deployment, or persistence beyond the sample CSV files.
- No generated Flex/Bison sources committed to the repository.
- No claim that documented SQL queries produce executed results yet.
