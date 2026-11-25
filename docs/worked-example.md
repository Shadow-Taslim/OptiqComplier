# Worked Example: Source → Tokens → AST → Trace → Output

This walkthrough demonstrates the full Week 1 design on one small program. The **source → AST** half is implemented and verified in Week 2; the **trace → output** half is the Week 3 interpreter design, shown here to validate the grammar and AST shape.

## 1. Source Program

```text
// Find how many salaries clear a computed bar
let threshold = 50000 + 10000;
let matches = SELECT name FROM employees WHERE salary >= threshold;
if (COUNT(matches) > 3) {
    print('large team');
} else {
    print('small team');
}
```

## 2. Token Stream (Abbreviated)

Produced by the Flex lexer; every token carries its line and column.

```text
LET(2:5)  IDENTIFIER(threshold)  EQUAL  INTEGER_LITERAL(50000)  PLUS
INTEGER_LITERAL(10000)  SEMICOLON
LET(3:5)  IDENTIFIER(matches)  EQUAL  SELECT  IDENTIFIER(name)  FROM
IDENTIFIER(employees)  WHERE  IDENTIFIER(salary)  GREATER_EQUAL
IDENTIFIER(threshold)  SEMICOLON
IF(4:1)  LEFT_PAREN  COUNT  LEFT_PAREN  IDENTIFIER(matches)  RIGHT_PAREN
GREATER  INTEGER_LITERAL(3)  RIGHT_PAREN  LEFT_BRACE ...
```

The `//` comment and all whitespace are consumed without producing tokens, while line/column tracking continues through them.

## 3. AST (Abbreviated JSON)

Produced by the Bison parser and the AST printer (`optiq_cli --ast`):

```json
{
  "type": "Program",
  "statements": [
    {
      "type": "LetStatement",
      "name": "threshold",
      "initializer": {
        "type": "BinaryExpression",
        "operator": "+",
        "left":  { "type": "LiteralExpression", "kind": "integer", "value": 50000 },
        "right": { "type": "LiteralExpression", "kind": "integer", "value": 10000 }
      }
    },
    {
      "type": "LetStatement",
      "name": "matches",
      "initializer": {
        "type": "QueryExpression",
        "query": {
          "type": "SelectStatement",
          "select": [ { "type": "SelectItem", "expression": { "type": "ColumnReference", "name": "name" }, "alias": null } ],
          "from": { "type": "TableReference", "name": "employees", "alias": null },
          "where": {
            "type": "WhereClause",
            "predicate": {
              "type": "BinaryExpression",
              "operator": ">=",
              "left":  { "type": "ColumnReference", "name": "salary" },
              "right": { "type": "ColumnReference", "name": "threshold" }
            }
          },
          "join": null, "groupBy": null, "orderBy": null, "limit": null
        }
      }
    },
    {
      "type": "IfStatement",
      "condition": {
        "type": "BinaryExpression",
        "operator": ">",
        "left": {
          "type": "FunctionCall",
          "function": "COUNT",
          "argument": { "type": "ColumnReference", "name": "matches" }
        },
        "right": { "type": "LiteralExpression", "kind": "integer", "value": 3 }
      },
      "then": [ { "type": "PrintStatement", "expression": { "type": "LiteralExpression", "kind": "string", "value": "large team" } } ],
      "else": [ { "type": "PrintStatement", "expression": { "type": "LiteralExpression", "kind": "string", "value": "small team" } } ]
    }
  ]
}
```

## 4. Execution Trace (Week 3 Design, Illustrative)

| Step | Action | Environment after step |
| --- | --- | --- |
| 1 | Fold `50000 + 10000` → `60000`; bind `threshold` | `threshold = 60000` |
| 2 | Run query: `salary >= 60000` matches 7 of 8 rows (all except Henry Lee, 47000) | `matches = table[1 column × 7 rows]` |
| 3 | `COUNT(matches)` → `7`; `7 > 3` → `true` | condition holds |
| 4 | Enter the `then` block | — |
| 5 | `print('large team')` | program output: `large team` |

The `else` block is skipped because the condition held.

## 5. Expected Output

```text
large team
```

## 6. Planned Optimizations Demonstrated by This Program

Once the optimizer milestone lands, this same program exercises all three documented passes:

1. **Constant folding:** `50000 + 10000` → `60000` at compile time.
2. **Predicate pushdown:** `salary >= 60000` moves onto the `employees` table scan before projection.
3. **Projection pruning:** only the `name` and `salary` columns are read from the CSV; `id`, `dept_id`, and `hire_date` never load.

These optimizations are **designed, not yet executed** — Week 2 delivers the lexer, parser, and AST that make them possible.
