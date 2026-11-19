# SQL Mini-Compiler with Query Optimization

An SQL mini-compiler and query optimization project using **Flex**, **Bison**, and **C++17**, with an **Express API** and **React frontend** planned for later milestones. The current compiler accepts the documented SQL subset, builds a strongly typed Abstract Syntax Tree (AST), and prints that AST as JSON.

> **Current status — Week 2:** lexical analysis, SQL parsing, typed AST generation, syntax diagnostics, the command-line interface, and automated frontend tests are implemented. Semantic analysis, relational planning, optimization, CSV execution, API handlers, and the React interface remain future work.

## Project Scope

The initial release targets the SQL behaviors captured by the 12 fixtures in [Supported Example SQL Queries](docs/example-queries.md): projection, filtering, constant arithmetic, boolean predicates, aliases, inner joins, aggregation, grouping, ordering, and limits. Invalid or unsupported syntax produces an error with a source line and column.

### Implemented Compiler Frontend

- **Flex lexer:** case-insensitive keywords, identifiers, qualified-name components, integer/decimal/string literals, operators, punctuation, whitespace, and line/block comments.
- **Bison parser:** `SELECT`, `FROM`, aliases, `JOIN`/`INNER JOIN`, `ON`, `WHERE`, `NOT`, `AND`, `OR`, arithmetic and comparison expressions, `GROUP BY`, `ORDER BY ASC/DESC`, `LIMIT`, and the five documented aggregates.
- **Typed AST:** dedicated C++17 nodes for statements, clauses, columns, literals, unary/binary expressions, and aggregate calls, with `std::unique_ptr` ownership.
- **AST printer:** deterministic JSON output for compiler inspection and tests.
- **CLI:** SQL from command-line arguments or standard input, AST output on success, and source-located diagnostics on failure.
- **Automated tests:** AST, token, grammar, precedence, invalid-input, CLI, and all 12 SQL fixture checks.

### Planned Later Capabilities

- **Semantic analysis:** catalog validation, name resolution, and type checking.
- **Relational Algebra Mapping:** Convert queries to relational operators ($\sigma$, $\pi$, $\bowtie$, $\gamma$, $\tau$, and Table Scan).
- **Optimization Passes:**
  - **Predicate Pushdown ($\sigma$):** Push filters toward base scans to reduce tuples before joins.
  - **Projection Pruning ($\pi$):** Remove unused columns early to reduce tuple width.
  - **Constant Folding:** Evaluate static arithmetic and boolean expressions at compile time.
- **Plan Inspection:** Provide JSON plan trees for later ASCII and Graphviz visualizations.
- **Execution Statistics:** Return timings, row counts, plan sizes, and optimization counts.

## Architecture Pipeline

### Working Week 2 frontend

```text
SQL argument or stdin
      │
      ▼
Flex lexer ──tokens + locations──> Bison parser ──> typed AST ──> JSON AST output
                                      │
                                      └──> source-located syntax error
```

### Planned application pipeline

```text
React SQL editor
      │ HTTP
      ▼
Express API ──JSON stdin──> C++ compiler process
                              │
                              ▼
SQL Query ──> Lexer ──> Parser ──> AST
      ──> Semantic Analysis ──> Planner ──> Initial Plan
      ──> Optimizer ──> Optimized Plan ──> CSV Executor for execute requests
                              │
React plans/results/stats <──JSON stdout── Express API
```

The future request/response contract, including errors, plans, results, and statistics, is defined in [Express ↔ C++ Compiler JSON Contract](docs/api-contract.md). The current Week 2 CLI prints AST JSON, not that compile/execute response envelope.

## Project Structure

```text
├── compiler/
│   ├── include/optiq/       # AST, AST printer, lexer, and parser interfaces
│   ├── src/
│   │   ├── lexer.l          # Flex token rules
│   │   ├── parser.y         # Bison SQL grammar
│   │   ├── cli.cpp          # optiq_cli entry point
│   │   └── ...              # Scanner, parser, and AST implementation
│   └── CMakeLists.txt       # C++17 targets, generated sources, and CTest
├── apps/
│   ├── api/                 # Future Express service and compiler adapter
│   └── web/                 # Future React + TypeScript + Vite client
├── tests/
│   ├── compiler/            # AST, lexer, parser, fixture, and CLI tests
│   ├── api/                 # Future API tests
│   ├── frontend/            # Future frontend tests
│   └── fixtures/            # Twelve unchanged documented SQL fixtures
├── docs/
│   ├── architecture.md
│   ├── api-contract.md
│   ├── example-queries.md
│   └── week2-verification.md
├── data/
│   ├── schemas/             # employees and departments metadata
│   ├── employees.csv
│   └── departments.csv
├── package.json             # npm workspaces for later application milestones
├── .editorconfig
├── .gitignore
└── README.md
```

See [Architecture and Implementation Plan](docs/architecture.md) for component responsibilities, ownership, grammar precedence, request flow, and later implementation order.

## Sample Catalog

The `sample-v1` catalog contains two related tables:

- **`employees`** — 8 rows with `id`, `name`, `dept_id`, `salary`, and `hire_date`.
- **`departments`** — 4 rows with `id`, `dept_name`, `location`, and `budget`.

`employees.dept_id` references `departments.id`. The JSON schema files define column types, keys, nullability, and CSV locations. CSV execution is not implemented in Week 2.

## Dependencies

### Native compiler

- CMake 3.20 or newer
- A C++17 compiler; MinGW-w64 is the initial Windows target
- Flex 2.6 or newer and Bison 3.0 or newer (`win_flex` / `win_bison` on Windows)

Flex and Bison generate sources inside the CMake build directory. Generated parser and scanner files are not committed.

### JavaScript workspaces for later milestones

- Node.js 20 or newer and npm 10 or newer
- Express 5
- React 19, TypeScript, and Vite

## Windows PowerShell Setup

Install CMake, MinGW-w64, and WinFlexBison, then ensure their executables are available on `PATH`:

```powershell
where.exe cmake
where.exe mingw32-make
where.exe win_flex
where.exe win_bison
```

Clone, configure, build, and test from the repository root:

```powershell
git clone https://github.com/Shadow-Taslim/OptiqComplier.git
cd OptiqComplier
cmake -S compiler -B build/compiler -G "MinGW Makefiles" -DOPTIQ_BUILD_TESTS=ON
cmake --build build/compiler --parallel
ctest --test-dir build/compiler --output-on-failure
```

The Week 2 implementation was built and tested with GNU C++ 12.2, Flex 2.6.4, Bison 3.8.2, and CMake 3.25.1 on Linux. The commands above are the native Windows target instructions; they should also be run on the intended Windows 10 machine.

## CLI Usage

Pass SQL as an argument:

```powershell
.\build\compiler\optiq_cli.exe "SELECT name, salary FROM employees WHERE salary >= 70000 ORDER BY salary DESC;"
```

Or provide SQL through standard input:

```powershell
Get-Content .\tests\fixtures\01-ordered-projection.sql -Raw | .\build\compiler\optiq_cli.exe
```

Valid SQL prints the AST as JSON to stdout. Invalid SQL prints a line/column diagnostic to stderr.

| Exit code | Meaning |
| --- | --- |
| `0` | SQL parsed successfully |
| `2` | Lexical or syntax error |
| `1` | Unexpected internal error |

Use `optiq_cli --help` (or `optiq_cli.exe --help` on Windows) for command help.

## Automated Verification

On a platform with the native toolchain available:

```sh
cmake -S compiler -B build/compiler -DOPTIQ_BUILD_TESTS=ON
cmake --build build/compiler --parallel
ctest --test-dir build/compiler --output-on-failure
```

The verified Week 2 suite contains **8 CTest tests**, including all **12 SQL fixtures**, and completes with no failures. See [Week 2 Verification](docs/week2-verification.md) for the exact build scope and limitations.

## Optimization Example

**Input query:**

```sql
SELECT e.name, d.dept_name
FROM employees e
JOIN departments d ON e.dept_id = d.id
WHERE e.salary > 20000 + 10000 AND d.location = 'HQ';
```

This query can currently be lexed, parsed, and represented as an AST. The following optimizations are **planned, not yet executed**:

1. **Constant Folding:** `20000 + 10000` → `30000`.
2. **Predicate Pushdown:** Move `e.salary > 30000` toward the `employees` scan and `d.location = 'HQ'` toward the `departments` scan.
3. **Projection Pruning:** Strip unused attributes before the join.

**Illustrative future plan:**

```text
Projection [e.name, d.dept_name]
└── Join [e.dept_id = d.id]
    ├── Selection [salary > 30000] ──> TableScan [employees]
    └── Selection [location = 'HQ'] ──> TableScan [departments]
```

## Week 1 Foundations

- Repository architecture and minimal workspace/CMake configuration.
- Versioned compile and execute JSON contract.
- Two table schemas and small CSV datasets.
- Twelve supported SQL examples with expected results.
- Setup and implementation-scope documentation.

## Week 2 Deliverables

- Working Flex lexer and conflict-free Bison parser.
- Strongly typed C++17 AST and JSON AST printer.
- Command-line compiler with argument/stdin modes and source-located errors.
- CMake-generated scanner/parser sources and automated CTest suite.
- Twelve reusable SQL acceptance fixtures.
- Updated architecture, setup, CLI, and verification documentation.

## License

MIT
