# Optiq Language Specification

**Language version:** `0.2`
**Status:** Week 1 definition; Week 2 implements the lexer, parser, and AST for this specification.

Optiq is a mini programming language compiled with **Flex** and **Bison**. SQL `SELECT` queries are a first-class part of the language: a query can appear as a statement or as an expression whose value is a table. This document is the single source of truth for the Week 2 grammar.

## 1. Design Goals

- Demonstrate a complete compiler frontend: lexical analysis (Flex), syntax analysis (Bison), typed AST construction, and source-located diagnostics.
- Provide the classic programming-language features: variables, assignment, expressions, conditionals, loops, and output.
- Integrate the existing SQL subset so programs can query the `sample-v1` CSV catalog without leaving the language.

## 2. Lexical Structure

### 2.1 Comments

| Form | Kind |
| --- | --- |
| `-- text` | SQL-style line comment |
| `// text` | C-style line comment |
| `/* text */` | Block comment, may span lines; unterminated blocks are an error |

### 2.2 Identifiers and Keywords

- Identifiers match `[A-Za-z_][A-Za-z0-9_]*`.
- Keywords are **case-insensitive** (`let`, `LET`, and `Let` are identical).
- Language keywords: `LET`, `IF`, `ELSE`, `WHILE`, `PRINT`, `TRUE`, `FALSE`.
- SQL keywords: `SELECT`, `FROM`, `AS`, `INNER`, `JOIN`, `ON`, `WHERE`, `GROUP`, `BY`, `ORDER`, `ASC`, `DESC`, `LIMIT`, `AND`, `OR`, `NOT`, `COUNT`, `SUM`, `AVG`, `MIN`, `MAX`.
- A keyword is never an identifier.

### 2.3 Literals

| Literal | Examples | Notes |
| --- | --- | --- |
| Integer | `42`, `70000` | Signed 64-bit range; out-of-range is a lexical error. |
| Decimal | `3.14`, `0.5` | Stored as double precision. |
| String | `'Aisha''s'` | Single-quoted; a doubled single quote escapes a quote. Unterminated strings are an error. |
| Boolean | `true`, `false` | The keywords `TRUE` and `FALSE`. |

### 2.4 Operators and Punctuation

`(` `)` `{` `}` `,` `;` `.` `=` `==` `!=` `<>` `<` `<=` `>` `>=` `+` `-` `*` `/`

A number directly followed by letters (for example `12abc`) is a lexical error, not two tokens.

## 3. Types

| Type | Produced by | Notes |
| --- | --- | --- |
| `integer` | Integer literals, arithmetic on integers | 64-bit signed. |
| `decimal` | Decimal literals, division, mixed arithmetic | Double precision. |
| `string` | String literals | Concatenation is not part of Week 2. |
| `boolean` | `TRUE`, `FALSE`, comparisons, `AND`/`OR`/`NOT` | Conditions require a boolean. |
| `table` | Query expressions | A collection of named columns and rows, executed against CSV data in Week 3. |

Static type checking belongs to the semantic-analysis milestone; Week 2 checks syntax only.

## 4. Statements

A program is a sequence of statements. Every simple statement ends with `;`. Compound statements use brace blocks and take no trailing semicolon.

### 4.1 Variable declaration

```text
let threshold = 5000;
```

Declares a variable in the current block and initializes it. Redeclaring a name in the same block is a semantic error (Week 3).

### 4.2 Assignment

```text
threshold = threshold + 1000;
```

Assigns to an existing variable. Assignment is a statement, never an expression, so `=` inside an expression is always a comparison.

### 4.3 Output

```text
print(threshold);
print('done');
```

Evaluates the expression and writes its value to program output.

### 4.4 Conditional

```text
if (count > 3) {
    print('large');
} else if (count > 0) {
    print('small');
} else {
    print('empty');
}
```

`else if` chains and a final `else` block are supported. The condition must be a boolean.

### 4.5 Loop

```text
while (threshold < 10000) {
    threshold = threshold + 1000;
}
```

Evaluates the body while the boolean condition holds.

### 4.6 Query statement

```text
SELECT name, salary FROM employees ORDER BY salary DESC;
```

A standalone query executes and its resulting table becomes program output (Week 3 semantics).

## 5. Expressions

From lowest to highest precedence, all binary levels left-associative:

1. `OR`
2. `AND`
3. Prefix `NOT`
4. Comparisons: `=`, `==`, `!=`, `<>`, `<`, `<=`, `>`, `>=` (non-associative)
5. Addition and subtraction: `+`, `-`
6. Multiplication and division: `*`, `/`
7. Unary `+` and `-`
8. Literals, name references, function calls, query expressions, and parenthesized expressions

### 5.1 Name references

A bare identifier (`salary`, `threshold`) or a qualified name (`e.salary`). Whether a name denotes a program variable or a query column is decided by semantic analysis: inside a query expression, unqualified names resolve to columns of the queried tables first and to program variables second.

### 5.2 Function calls

`COUNT`, `SUM`, `AVG`, `MIN`, and `MAX` take one expression argument; `COUNT(*)` counts rows. Inside a query they are SQL aggregates; applied to a table variable they compute over its rows.

### 5.3 Query expressions

```text
let seniors = SELECT name FROM employees WHERE salary > threshold;
```

A `SELECT` query can be used as a **query expression** whose value is a `table`. To keep the grammar unambiguous (LALR conflict-free), a query expression may appear in exactly two positions:

- as the entire right-hand side of a `LET` declaration or assignment, and
- as a standalone query statement.

Deeper nesting (for example `print(SELECT ...)`) is rejected as a syntax error; bind the query to a variable first and pass the variable instead. The SQL subset itself is unchanged from the Week 2 SQL frontend: one base table with optional alias, zero or one `JOIN`/`INNER JOIN ... ON`, and optional `WHERE`, `GROUP BY`, `ORDER BY`, `LIMIT` clauses.

## 6. Grammar (EBNF)

```ebnf
program            = statement_list ;

statement_list     = { statement } ;

statement          = let_statement
                   | assign_statement
                   | print_statement
                   | if_statement
                   | while_statement
                   | query_statement ;

let_statement      = "LET" identifier "=" ( expression | select_statement ) ";" ;
assign_statement   = identifier "=" ( expression | select_statement ) ";" ;
print_statement    = "PRINT" "(" expression ")" ";" ;

if_statement       = "IF" "(" expression ")" block [ else_part ] ;
else_part          = "ELSE" block
                   | "ELSE" if_statement ;

while_statement    = "WHILE" "(" expression ")" block ;
block              = "{" statement_list "}" ;

query_statement    = select_statement ";" ;  (* the semicolon is required in programs *)

expression         = or_expression ;
or_expression      = and_expression { "OR" and_expression } ;
and_expression     = not_expression { "AND" not_expression } ;
not_expression     = [ "NOT" ] predicate ;
predicate          = additive_expression [ comparison_operator additive_expression ] ;
comparison_operator = "=" | "==" | "!=" | "<>" | "<" | "<=" | ">" | ">=" ;
additive_expression       = multiplicative_expression { ("+" | "-") multiplicative_expression } ;
multiplicative_expression = unary_expression { ("*" | "/") unary_expression } ;
unary_expression   = [ "+" | "-" ] unary_expression | primary_expression ;
primary_expression = identifier [ "." identifier ]
                   | integer_literal
                   | decimal_literal
                   | string_literal
                   | "TRUE"
                   | "FALSE"
                   | function_call
                   | "(" expression ")" ;

function_call      = "COUNT" "(" "*" ")"
                   | aggregate_function "(" expression ")" ;
aggregate_function = "COUNT" | "SUM" | "AVG" | "MIN" | "MAX" ;

select_statement   = "SELECT" select_list "FROM" table_reference
                     [ join_clause ] [ where_clause ] [ group_by_clause ]
                     [ order_by_clause ] [ limit_clause ] ;
select_list        = select_item { "," select_item } ;
select_item        = "*" | expression [ [ "AS" ] identifier ] ;
table_reference    = identifier [ [ "AS" ] identifier ] ;
join_clause        = [ "INNER" ] "JOIN" table_reference "ON" expression ;
where_clause       = "WHERE" expression ;
group_by_clause    = "GROUP" "BY" identifier_or_qualified { "," identifier_or_qualified } ;
order_by_clause    = "ORDER" "BY" order_item { "," order_item } ;
order_item         = identifier_or_qualified [ "ASC" | "DESC" ] ;
limit_clause       = "LIMIT" integer_literal ;
identifier_or_qualified = identifier [ "." identifier ] ;
```

## 7. Error Handling

- Lexical errors: invalid characters, malformed or out-of-range numbers, unterminated strings, unterminated block comments.
- Syntax errors: verbose Bison messages with the line and column of the unexpected token.
- The CLI reports `error at line L, column C: message` on stderr and exits with code `2`.

## 8. Example Program

```text
// Raise the threshold until only senior salaries remain
let threshold = 50000;
while (threshold < 100000) {
    threshold = threshold + 10000;
}

let seniors = SELECT name, salary FROM employees WHERE salary >= threshold;
print(threshold);
print(seniors);
```

Printing a `table` value writes its rows; binding query results to variables is Week 3 semantics, but the syntax above is final.

## 9. Explicit Non-Goals for Week 2

- No execution, type checking, or name resolution — syntax and AST only.
- No `for` loops, functions, arrays, or string concatenation.
- No SQL features beyond the documented subset (no subqueries, `INSERT`, `UPDATE`, or multiple joins).
