# Architecture and Week 1 Plan

## Status

Week 1 establishes the repository layout, interface contract, sample catalog, and acceptance fixtures only. The lexer, parser, optimizer, executor, API handlers, and React interface are intentionally not implemented.

## Components

1. **C++ compiler (`compiler/`)** — Flex/Bison front end, typed AST, semantic checks, relational-algebra planner, optimizer, and later a small CSV execution engine. It is a standalone process with no Express or React dependencies.
2. **Express API (`apps/api/`)** — Validates browser requests, starts the compiler process, forwards one JSON request over stdin, and returns the compiler's JSON response. It owns process timeouts and safe error propagation.
3. **React frontend (`apps/web/`)** — Will provide SQL input, compile/execute actions, initial/optimized plan views, tabular results, errors, and statistics.
4. **Sample catalog (`data/`)** — Versioned table metadata and small CSV datasets used consistently by the compiler, API, and tests.
5. **Tests and documentation (`tests/`, `docs/`)** — Contract fixtures, future component/integration tests, architecture notes, and expected SQL results.

## Folder Structure

```text
OptiqComplier/
├── compiler/
│   ├── include/              # Public C++ headers
│   ├── src/                  # Flex, Bison, AST, planner, optimizer, executor
│   └── CMakeLists.txt        # C++17 project configuration; targets come later
├── apps/
│   ├── api/
│   │   ├── src/              # Express routes and compiler process adapter
│   │   ├── package.json
│   │   └── tsconfig.json
│   └── web/
│       ├── src/              # React components, plan/result views, API client
│       ├── package.json
│       ├── tsconfig.json
│       └── vite.config.ts
├── tests/
│   ├── compiler/             # Lexer, parser, optimizer, and execution tests
│   ├── api/                  # Contract and process-adapter tests
│   ├── frontend/             # Component and end-to-end tests
│   └── fixtures/             # Shared request/response JSON fixtures
├── docs/
│   ├── architecture.md
│   ├── api-contract.md
│   └── example-queries.md
├── data/
│   ├── schemas/              # employees and departments table metadata
│   ├── employees.csv
│   └── departments.csv
├── package.json              # npm workspaces for API and web
├── .editorconfig
├── .gitignore
└── README.md
```

## Request Flow

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

The browser never starts the compiler directly. Express never passes arbitrary filesystem paths from the browser; the catalog identifier is allow-listed. The full message shapes and failure rules are in [api-contract.md](api-contract.md).

## Initial SQL Boundary

The sample fixtures establish the first implementation target: single-table projection, filters with arithmetic and boolean expressions, inner joins, aliases, the five basic aggregates, `GROUP BY`, `ORDER BY`, and `LIMIT`. Unsupported SQL must produce a structured `UNSUPPORTED_SQL` error rather than be silently ignored.

## Later Implementation Order

1. Add lexer/parser/AST targets and compiler unit tests.
2. Add semantic checks, relational planning, and the three optimization passes.
3. Add CSV execution and verify all documented query results.
4. Add the Express adapter and API integration tests.
5. Add the React workflow and end-to-end tests.

## Explicit Non-Goals for Week 1

- No compiler source or generated parser files.
- No Express routes, React pages, or UI components.
- No database, authentication, deployment, or persistence beyond the sample CSV files.
- No claim that the documented SQL is executable yet.
