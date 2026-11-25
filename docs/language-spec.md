# Language Specification

This document formally specifies the language implemented by Optiq, exactly as
defined in **Section 5 of the Compiler Construction Lab Project Manual**
(Metropolitan University, CSE). The manual is the authoritative specification;
this document restates it precisely and records the formal context-free grammar
and the semantic rules enforced by the compiler.

Source files use the `.mc` extension (plain `.txt` also works).

## 1. Lexical Structure

### 1.1 Keywords

```
int  float  bool  if  else  while  print  true  false
```

Keywords are lowercase and case-sensitive (`Int` is an identifier, not a
keyword), matching C-family convention.

### 1.2 Identifiers

```
identifier ::= (letter | "_") (letter | digit | "_")*
```

### 1.3 Literals

- Integer literals: `42`, `0`, `1000`
- Floating-point literals: `3.14`, `0.5`
- Boolean literals: `true`, `false`

### 1.4 Operators

| Category   | Operators                          |
| ---------- | ---------------------------------- |
| Arithmetic | `+` `-` `*` `/` `%`                |
| Relational | `<` `>` `<=` `>=` `==` `!=`        |
| Logical    | `&&` `||` `!`                      |

`=` is the assignment operator.

### 1.5 Delimiters

`{` `}` `(` `)` `;` — semicolons terminate declarations, assignments, and
print statements.

### 1.6 Comments and Whitespace

- Single-line comments start with `//` and run to end of line.
- Block comments are enclosed in `/*` ... `*/` and do not nest.
- Comments and whitespace (spaces, tabs, newlines) are discarded, never
  tokenized.
- Any character sequence that matches no valid token is reported as a lexical
  error with its line number.

## 2. Formal Context-Free Grammar

Terminals are shown in **bold** or quoted; the start symbol is `program`.

```
program          → statement_list

statement_list   → statement_list statement
                 | ε

statement        → declaration
                 | assignment
                 | if_statement
                 | while_statement
                 | print_statement
                 | block

declaration      → type IDENTIFIER ";"
                 | type IDENTIFIER "=" expression ";"

type             → "int" | "float" | "bool"

assignment       → IDENTIFIER "=" expression ";"

if_statement     → "if" "(" expression ")" block
                 | "if" "(" expression ")" block "else" block

while_statement  → "while" "(" expression ")" block

print_statement  → "print" expression ";"

block            → "{" statement_list "}"

expression       → logic_or
logic_or         → logic_or "||" logic_and | logic_and
logic_and        → logic_and "&&" equality | equality
equality         → equality "==" relational
                 | equality "!=" relational
                 | relational
relational       → relational "<" additive
                 | relational ">" additive
                 | relational "<=" additive
                 | relational ">=" additive
                 | additive
additive         → additive "+" multiplicative
                 | additive "-" multiplicative
                 | multiplicative
multiplicative   → multiplicative "*" unary
                 | multiplicative "/" unary
                 | multiplicative "%" unary
                 | unary
unary            → "-" unary | "!" unary | primary
primary          → IDENTIFIER
                 | INTEGER_LITERAL
                 | FLOAT_LITERAL
                 | "true"
                 | "false"
                 | "(" expression ")"
```

### 2.1 Precedence and Associativity

From loosest to tightest binding; all binary operators are left-associative:

1. `||`
2. `&&`
3. `==` `!=`
4. `<` `>` `<=` `>=`
5. `+` `-`
6. `*` `/` `%`
7. unary `-` `!` (right-associative)
8. parentheses, literals, identifiers

There is no dangling-else ambiguity: both branches of `if` are mandatory
blocks, so the grammar is unambiguous and LALR(1) conflict-free.

## 3. Semantic Rules

The grammar above is context-free; the following rules are enforced by the
semantic analyzer (symbol table + type checking), with line-numbered errors:

| # | Rule | Example of violation |
| - | ---- | -------------------- |
| 1 | A variable must be declared before use. | `x = 5;` with no `int x;` |
| 2 | A variable may not be redeclared in the same scope. | `int x; int x;` |
| 3 | A variable declared inside a block is not visible outside it. | `if (true) { int y; } y = 2;` |
| 4 | A declaration initializer must match the declared type. | `bool b = 5 + 3.2;` |
| 5 | An assignment value must match the variable's type. | `int x; x = true;` |
| 6 | `&&`, `\|\|`, `!` require `bool` operands. | `bool b = 1 && 2;` |

### 3.1 Type System

- `int`, `float`, `bool` are distinct types.
- Arithmetic `+ - * /` requires numeric (`int`/`float`) operands; the result is
  `float` if either operand is `float`, otherwise `int`.
- `%` requires both operands to be `int` and produces `int`.
- Relational operators require numeric operands and produce `bool`.
- `==`/`!=` require matching operand types (numeric-with-numeric or
  `bool`-with-`bool`) and produce `bool`.
- Logical operators require `bool` operands and produce `bool`.
- Unary `-` requires a numeric operand; unary `!` requires a `bool` operand.
- `if`/`while` conditions must be `bool`.
- The only implicit conversion is the widening `int` → `float`, allowed in
  initializers, assignments, and mixed arithmetic. Everything else must match
  exactly.

### 3.2 Scoping

Every block `{ ... }` introduces a new scope. Lookup proceeds from the
innermost scope outward, so an inner declaration shadows an outer one; when the
block ends, its declarations cease to exist.

## 4. Three Address Code (Target of Compilation)

The compiler's final output is TAC with these conventions:

- Temporaries `t1, t2, ...` hold intermediate expression values.
- Labels `L1, L2, ...` mark jump targets (`L1:` on its own line).
- Conditional jumps: `ifFalse <cond> goto <label>`.
- Unconditional jumps: `goto <label>`.
- Output: `print <operand>`.
- `while` loops compile to a start label, a conditional exit, the body, and a
  backward jump; `if-else` compiles to a conditional jump to the else block
  and an unconditional jump over it.

Example — for the source

```
int a;
int b;
int c;
a = 5;
b = 10;
c = a + b * 2;
```

the compiler emits

```
a = 5
b = 10
t1 = b * 2
t2 = a + t1
c = t2
```
