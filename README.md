# SQL Mini-Compiler with Query Optimization

An SQL compiler and query optimization engine implemented with **Flex**, **Bison**, and **C/C++**. It parses SQL queries into an Abstract Syntax Tree (AST), translates them into Relational Algebra, and applies heuristic optimizations (Predicate Pushdown, Projection Pruning, and Constant Folding).

---

## Architecture Pipeline

```text
SQL Query ──> [Flex: Lexer] ──> [Bison: Parser] ──> AST ──> [Planner] ──> Initial Plan ──> [Optimizer] ──> Optimized Plan
```

---

## Features

- **Lexer & Parser:** Tokenizes and parses standard SQL (`SELECT`, `FROM`, `JOIN`, `WHERE`, `GROUP BY`, `ORDER BY`, `LIMIT`).
- **Relational Algebra Mapping:** Converts queries to relational operators ($\sigma$, $\pi$, $\bowtie$, $\gamma$, $\tau$, Table Scan).
- **Optimization Passes:**
  - **Predicate Pushdown ($\sigma$):** Pushes filter conditions down to base scans, reducing tuples before joins.
  - **Projection Pruning ($\pi$):** Eliminates unused columns early to reduce tuple width and memory usage.
  - **Constant Folding:** Evaluates static arithmetic (`10 + 5` $\to$ `15`) and boolean tautologies at compile time.
- **Plan Inspection:** Generates ASCII tree views and Graphviz DOT files for plan comparison.

---

## Project Structure

```text
├── src/
│   ├── lexer.l          # Flex token rules
│   ├── parser.y         # Bison grammar & AST generation
│   ├── ast.c / .h       # AST node constructors & structures
│   ├── catalog.c / .h   # In-memory schema and type checking
│   ├── planner.c / .h   # Relational algebra plan generation
│   ├── optimizer.c / .h # Heuristic optimization passes
│   └── main.c           # CLI driver & REPL
├── tests/               # Test queries
├── Makefile             # Build automation
└── README.md
```

---

## Quick Start

### Prerequisites
- **GCC / Clang**
- **Flex** (`>= 2.6`)
- **Bison** (`>= 3.0`)
- **Make**

### Build
```bash
make
```

### Run
```bash
# Interactive REPL
./sql_compiler

# Compile a file with optimization & explanation
./sql_compiler -f query.sql --optimize --explain
```

---

## Optimization Example

**Input Query:**
```sql
SELECT e.name, d.dept_name
FROM employees e
JOIN departments d ON e.dept_id = d.id
WHERE e.salary > 20000 + 10000 AND d.location = 'HQ';
```

**Optimizations Applied:**
1. **Constant Folding:** `20000 + 10000` $\to$ `30000`.
2. **Predicate Pushdown:** 
   - `e.salary > 30000` pushed directly to `employees` scan.
   - `d.location = 'HQ'` pushed directly to `departments` scan.
3. **Projection Pruning:** Unused attributes are stripped before the join.

**Resulting Plan:**
```text
Projection [e.name, d.dept_name]
└── Join [e.dept_id = d.id]
    ├── Selection [salary > 30000] ──> TableScan [employees]
    └── Selection [location = 'HQ'] ──> TableScan [departments]
```

---

## License

MIT
