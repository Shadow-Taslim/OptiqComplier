# SQL Mini-Compiler with Query Optimization

An SQL compiler and query optimization engine planned with **Flex**, **Bison**, and **C++**, exposed through an **Express API** and a **React frontend**. It will parse SQL queries into an Abstract Syntax Tree (AST), translate them into Relational Algebra, apply heuristic optimizations, and execute the supported subset against small CSV datasets.

> **Current status — Week 1:** project planning, interface design, sample data, and initial scaffolding only. Compiler and application behavior are not implemented yet.

## Project Scope

The initial release targets the SQL behaviors captured by the 12 fixtures in [Supported Example SQL Queries](docs/example-queries.md): projection, filtering, constant arithmetic, boolean predicates, aliases, inner joins, aggregation, grouping, ordering, and limits. Unsupported statements must return explicit structured errors.

### Planned Compiler Capabilities

- **Lexer & Parser:** Tokenize and parse `SELECT`, `FROM`, `JOIN`, `WHERE`, `GROUP BY`, `ORDER BY`, and `LIMIT`.
- **Relational Algebra Mapping:** Convert queries to relational operators ($\sigma$, $\pi$, $\bowtie$, $\gamma$, $\tau$, and Table Scan).
- **Optimization Passes:**
  - **Predicate Pushdown ($\sigma$):** Push filters toward base scans to reduce tuples before joins.
  - **Projection Pruning ($\pi$):** Remove unused columns early to reduce tuple width.
  - **Constant Folding:** Evaluate static arithmetic and boolean expressions at compile time.
- **Plan Inspection:** Provide JSON plan trees for later ASCII and Graphviz visualizations.
- **Execution Statistics:** Return timings, row counts, plan sizes, and optimization counts.

## Architecture Pipeline

```text
React SQL editor
      │ HTTP
      ▼
Express API ──JSON stdin──> C++ compiler process
                              │
                              ▼
SQL Query ──> [Flex: Lexer] ──> [Bison: Parser] ──> AST
      ──> [Planner] ──> Initial Plan ──> [Optimizer] ──> Optimized Plan
      ──> [CSV Executor for execute requests]
                              │
React plans/results/stats <──JSON stdout── Express API
```

The versioned request/response contract, including errors, plans, results, and statistics, is defined in [Express ↔ C++ Compiler JSON Contract](docs/api-contract.md).

## Project Structure

```text
├── compiler/             # C++17 compiler, planner, optimizer, and executor
│   ├── include/
│   ├── src/
│   └── CMakeLists.txt
├── apps/
│   ├── api/              # Express service and compiler process adapter
│   └── web/              # React + TypeScript + Vite client
├── tests/
│   ├── compiler/
│   ├── api/
│   ├── frontend/
│   └── fixtures/
├── docs/
│   ├── architecture.md
│   ├── api-contract.md
│   └── example-queries.md
├── data/
│   ├── schemas/          # employees and departments metadata
│   ├── employees.csv
│   └── departments.csv
├── package.json          # npm workspaces
├── .editorconfig
├── .gitignore
└── README.md
```

See [Architecture and Week 1 Plan](docs/architecture.md) for component responsibilities, the request flow, implementation order, and explicit non-goals.

## Sample Catalog

The `sample-v1` catalog contains two related tables:

- **`employees`** — 8 rows with `id`, `name`, `dept_id`, `salary`, and `hire_date`.
- **`departments`** — 4 rows with `id`, `dept_name`, `location`, and `budget`.

`employees.dept_id` references `departments.id`. The JSON schema files define column types, keys, nullability, and CSV locations.

## Dependencies

### JavaScript

- Node.js 20 or newer and npm 10 or newer
- Express 5
- React 19, TypeScript, and Vite

### Native compiler

- CMake 3.20 or newer
- A C++17 compiler; MinGW-w64 is the initial Windows target
- Flex 2.6 or newer and Bison 3.0 or newer (`win_flex` / `win_bison` on Windows)

No native source targets exist yet, so Flex and Bison are not invoked by the Week 1 CMake configuration.

## Setup Outline

Use Windows PowerShell from the repository root:

```powershell
git clone https://github.com/Shadow-Taslim/OptiqComplier.git
cd OptiqComplier
npm install
cmake -S compiler -B build/compiler -G "MinGW Makefiles"
```

These commands install workspace dependencies and validate the initial CMake project. Development scripts, native targets, API startup, and frontend startup will be added with their implementations in later iterations.

## Optimization Example

**Input query:**

```sql
SELECT e.name, d.dept_name
FROM employees e
JOIN departments d ON e.dept_id = d.id
WHERE e.salary > 20000 + 10000 AND d.location = 'HQ';
```

**Planned optimizations:**

1. **Constant Folding:** `20000 + 10000` → `30000`.
2. **Predicate Pushdown:** Move `e.salary > 30000` toward the `employees` scan and `d.location = 'HQ'` toward the `departments` scan.
3. **Projection Pruning:** Strip unused attributes before the join.

**Illustrative resulting plan:**

```text
Projection [e.name, d.dept_name]
└── Join [e.dept_id = d.id]
    ├── Selection [salary > 30000] ──> TableScan [employees]
    └── Selection [location = 'HQ'] ──> TableScan [departments]
```

## Week 1 Deliverables

- Repository architecture and minimal workspace/CMake configuration.
- Versioned compile and execute JSON contract.
- Two table schemas and small CSV datasets.
- Twelve supported SQL examples with expected results.
- Setup and implementation-scope documentation.

## License

MIT
