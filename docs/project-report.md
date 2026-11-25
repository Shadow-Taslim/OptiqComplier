# Optiq: A Mini-Language Compiler Using Flex and Bison

**Course:** Compiler Construction Lab  
**Department:** Computer Science and Engineering  
**University:** Metropolitan University, Bangladesh  
**Repository:** https://github.com/Shadow-Taslim/OptiqComplier

| Submission field | Information to complete |
| --- | --- |
| Team member names and IDs | **[Add every member's name and student ID]** |
| Course section | **[Add section]** |
| Instructor | **[Add instructor name]** |
| Submission date | **[Add submission date]** |

AI-assisted development is disclosed in accordance with Section 10 of the
project manual. Every team member must review and understand the complete
implementation before the presentation and individual viva.

## 1. Introduction

A compiler front-end translates source text into a structured representation,
checks that the program is meaningful, and produces an intermediate form suitable
for later code generation. The phases are often studied separately, but their
dependencies become clearest when they are integrated into one working system.

Optiq implements this integration for the language specified in the Compiler
Construction Lab Project Manual. It accepts `.mc` source files and performs four
principal stages:

1. Flex-based lexical analysis produces a token stream with source locations.
2. Bison-based syntax analysis validates the context-free grammar and constructs
   an Abstract Syntax Tree (AST).
3. Semantic analysis uses a nested-scope symbol table and type rules to validate
   the AST.
4. Intermediate code generation translates valid programs into Three Address
   Code (TAC).

The project deliberately stops at TAC. Machine code, assembly, linking, register
allocation, optimization, and executable generation are outside the required
scope. Rejecting invalid programs with precise lexical, syntax, and semantic
diagnostics is treated as an essential compiler function, not as a secondary
feature.

## 2. Objectives

The project sets out to:

- implement every required language feature using a modular C++17 design;
- define and implement a complete, unambiguous context-free grammar;
- preserve source line and column information for useful diagnostics;
- construct a typed AST with deterministic text and JSON representations;
- support nested scopes, shadowing, redeclaration detection, and scope exit;
- enforce declaration, assignment, operator, and condition type rules;
- generate correct TAC for expressions, `if`, `if-else`, `while`, and `print`;
- provide valid and invalid sample programs with expected and actual outputs;
- automate verification with unit, golden-file, and command-line tests; and
- document reproducible Linux and Windows 10 build procedures.

## 3. Language Specification

### 3.1 Data types and lexical elements

The language has three types: `int`, `float`, and `bool`. Its reserved words are:

```text
int  float  bool  if  else  while  print  true  false
```

Identifiers begin with a letter or underscore and continue with letters, digits,
or underscores. Literals include integers such as `42`, floating-point values
such as `3.14`, and the Boolean values `true` and `false`.

Declarations, assignments, and print statements end with semicolons. Braces
create nested blocks, and parentheses group expressions and control conditions.
The implementation also discards whitespace, `//` line comments, and non-nested
`/* ... */` block comments.

### 3.2 Complete context-free grammar

The start symbol is `program`; `ε` denotes the empty production. Uppercase names
are tokens supplied by the lexer.

```text
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

### 3.3 Precedence and associativity

The expression hierarchy removes ambiguity by construction. From lowest to
highest precedence, the levels are `||`; `&&`; `==` and `!=`; relational
operators; `+` and `-`; `*`, `/`, and `%`; unary `-` and `!`; and finally primary
expressions. Binary operators are left-associative, while unary operators are
right-associative. Mandatory braces after `if`, `else`, and `while` eliminate the
usual dangling-`else` ambiguity.

### 3.4 Semantic rules

A name must be declared before use and may be declared only once in the same
scope. Inner declarations may shadow outer declarations, but a block-local name
is unavailable after its block ends. Exact-type initialization and assignment
are accepted; the only implicit conversion is widening from `int` to `float`.

Arithmetic `+`, `-`, `*`, and `/` require numeric operands. `%` requires two
`int` operands. Relational operators require numeric operands and return `bool`.
Equality accepts numeric-with-numeric or `bool`-with-`bool`. Logical operators
and unary `!` require Boolean operands. Every `if` and `while` condition must be
`bool`.

## 4. Compiler Architecture

The phases communicate through explicit data structures rather than shared
parser actions:

```text
Source code (.mc / .txt)
        │
        ▼
Flex lexical analyzer
        │  Token stream + source locations
        ▼
Bison syntax analyzer
        │  Abstract Syntax Tree
        ▼
Semantic analyzer + nested symbol table
        │  Validated AST
        ▼
TAC generator
        │
        ▼
Three Address Code
```

The lexer first produces a complete token vector. `TokenStream` then adapts that
vector to Bison's `yylex` interface. This separation allows the scanner and
parser to be tested independently and avoids global scanner state.

The AST is the common interface between syntax analysis, semantic analysis, and
code generation. Semantic analysis is intentionally a separate pass: it does not
modify the grammar to enforce context-sensitive rules. TAC generation runs only
when no lexical, syntax, or semantic error has been reported.

The command-line interface exposes each stage through `--tokens`, `--ast`,
`--json`, and `--check`; its default mode emits TAC. Input may come from a file,
inline arguments, or standard input.

## 5. Lexer Design

The lexer is implemented in `compiler/src/lexer.l` using a reentrant Flex
scanner. `ScannerState` owns the token vector, optional error, current location,
and next location. `YY_USER_ACTION` updates source positions before each rule
executes.

Important patterns are:

| Token class | Flex pattern or recognition method |
| --- | --- |
| Whitespace | `[ \t\r\n]+` |
| Line comment | `"//"[^\n]*` |
| Block comment | Start with `"/*"` and consume the `BLOCK_COMMENT` state through `"*/"` |
| Invalid numeric name | `[0-9]+(\.[0-9]+)?[A-Za-z_][A-Za-z0-9_]*` |
| Float literal | `[0-9]+\.[0-9]+` |
| Integer literal | `[0-9]+` |
| Keyword or identifier | `[A-Za-z_][A-Za-z0-9_]*`, followed by keyword-table lookup |
| Operators | Longest-match rules for `&&`, `||`, `==`, `!=`, `<=`, `>=`, then single-character operators |
| Delimiters | `(`, `)`, `{`, `}`, and `;` |

Flex's longest-match rule ensures that `<=` is not split into `<` and `=`.
Identifiers are initially recognized by one generic pattern; a case-sensitive
keyword table distinguishes reserved words. Integer conversion uses
`std::from_chars`, and floating-point conversion checks complete consumption and
finiteness. Invalid characters, malformed numeric literals, out-of-range values,
and unterminated block comments produce located lexical diagnostics.

## 6. Parser Design

The Bison grammar in `compiler/src/parser.y` directly implements the formal CFG.
It uses a pure parser, `%locations`, and verbose syntax errors. `parser.cpp`
maps each `TokenKind` to its Bison token and transfers identifier or literal
values through `YYSTYPE`.

Precedence is encoded by separate nonterminal levels instead of by suppressing
shift/reduce warnings. Consequently, `a + b * 2` is parsed as `a + (b * 2)`, and
`a - b - c` is parsed as `(a - b) - c`. Bison generation uses `--warnings=all`;
the verified grammar has no conflicts.

Basic recovery uses:

```text
statement → error SEMICOLON
```

with `yyerrok`. After a malformed simple statement, parsing resumes at the next
semicolon so independent later errors can be reported. Recovered null statements
are discarded when the final `Program` is assembled. Any syntax error prevents
semantic analysis and TAC generation, even if a partial tree was recoverable.

## 7. Abstract Syntax Tree

The AST is a typed C++17 hierarchy defined in `compiler/include/optiq/ast.hpp`.
Every expression and statement stores a `SourceLocation` and a node kind. Nodes
own their children with `std::unique_ptr`, giving deterministic destruction and
preventing accidental sharing or copying.

Expression nodes are:

- `IdentifierExpression` for variable references;
- `LiteralExpression`, whose `std::variant` stores an integer, float, or Boolean;
- `UnaryExpression` for negation and logical NOT; and
- `BinaryExpression` for arithmetic, relational, equality, and logical operators.

Statement nodes are `DeclarationStatement`, `AssignmentStatement`,
`IfStatement`, `WhileStatement`, `PrintStatement`, and `BlockStatement`.
Declarations contain an optional initializer. `IfStatement` owns a required
then-block and an optional else-block, while `WhileStatement` owns its body.
A `Program` owns the top-level statement list.

Bison actions construct nodes only after their children have been successfully
parsed. `%destructor` rules release discarded semantic values during error
handling. The AST can be printed as readable indented text with `--ast` or as
JSON with `--json`; both formats omit punctuation that has no structural meaning.

For example, `c = a + b * 2;` has an assignment whose value is an `Add` node.
Its left child is identifier `a`, and its right child is a `Multiply` node with
children `b` and `2`.

## 8. Semantic Analysis

Semantic analysis is implemented as an AST traversal in
`compiler/src/semantic.cpp`. The internal type lattice is `Int`, `Float`,
`Bool`, and `Unknown`. `Unknown` records that an earlier expression error has
already been reported and suppresses repetitive cascading diagnostics.

| Rule | Detection strategy |
| --- | --- |
| Declaration before use | Look up every assigned or referenced identifier; failure produces an undeclared-variable error. |
| Same-scope redeclaration | Insert into the current scope only; insertion failure identifies a duplicate. |
| Block visibility | Push a scope on block entry and pop it on exit, so later lookup cannot see local declarations. |
| Initializer compatibility | Compare the declared type with the initializer's inferred type using the assignment-compatibility rule. |
| Assignment compatibility | Compare the target symbol's type with the value type; permit exact matches and `int` to `float`. |
| Arithmetic operands | Require numeric operands for `+ - * /`; infer `float` if either side is `float`, otherwise `int`. |
| Modulo operands | Require `int` on both sides of `%` and infer `int`. |
| Relational operands | Require numeric operands and infer `bool`. |
| Equality operands | Require equal types or two numeric types; infer `bool`. |
| Logical operands | Require `bool` for both operands of `&&` and `||`. |
| Unary operands | Require numeric input for unary `-` and Boolean input for `!`. |
| Control conditions | Require the expression used by `if` or `while` to infer `bool`. |

The analyzer accumulates independent errors instead of throwing on the first
one. For example, `x = 5;` without a declaration is rejected with:

```text
Semantic error at line 1, column 1: Undeclared variable 'x' (not visible in this scope)
```

Because the analyzer only validates and infers types, TAC generation receives an
AST that has already passed all language-specific semantic checks.

## 9. Symbol Table

The symbol table combines a vector of scopes with per-scope hash maps. Each
`Symbol` stores the four required fields:

```text
name, ValueType, scope ID, declaration line
```

`enterScope()` appends a new map and a unique scope ID. `declare()` searches only
the current map: finding the same name there is a redeclaration, while a matching
name in an outer map is legal shadowing. `lookup()` iterates from the innermost
map toward the global map and returns the first match. `exitScope()` removes the
current map and its declarations.

This design gives fast expected lookup within each scope while preserving the
required nested-scope behavior. The semantic analyzer creates the global scope
explicitly and creates one child scope for each block.

Example:

```c
int x;
{
  float x;   // legal shadowing in a child scope
  x = 1;     // resolves to the inner float
}
x = 2;       // resolves to the outer int
```

After the inner block, its `float x` no longer participates in lookup.

## 10. Intermediate Code

TAC generation is a second AST traversal implemented in `compiler/src/tac.cpp`.
`newTemp()` allocates `t1`, `t2`, and so on; `newLabel()` allocates `L1`, `L2`,
and so on. Identifiers and literals can be used directly as operands. Each unary
or binary expression emits an instruction into a fresh temporary and returns
that temporary to its parent.

For:

```c
int a;
int b;
int c;
a = 5;
b = 10;
c = a + b * 2;
```

Optiq emits:

```text
a = 5
b = 10
t1 = b * 2
t2 = a + t1
c = t2
```

An `if-else` evaluates its condition, emits `ifFalse condition goto elseLabel`,
emits the then-block, jumps over the else-block, and places the required labels.
A `while` places a start label before its condition, emits a conditional exit,
emits the body, and uses a backward `goto` to reevaluate the condition.

The complete manual sample in `examples/06-manual-sample.mc` produces:

```text
x = 10
y = 0
flag = true
L1:
t1 = x > 0
ifFalse t1 goto L2
t2 = y + x
y = t2
t3 = x - 1
x = t3
goto L1
L2:
t4 = flag == true
ifFalse t4 goto L3
print y
goto L4
L3:
print x
L4:
```

No interpreter or backend is included because TAC is the manual's final required
output.

## 11. Challenges

### 11.1 Preserving unambiguous expression structure

A flat expression grammar would leave precedence and associativity unresolved.
The implementation instead uses a layered grammar, making the desired AST shape
part of the grammar itself. Parser tests check precedence and associativity, and
Bison is run with all warnings enabled rather than suppressing conflicts.

### 11.2 Recovering from syntax errors without accepting the program

Stopping at the first syntax error would hide later mistakes, but returning a
partial AST as valid could allow an invalid program to reach code generation.
The parser therefore records every diagnostic, recovers at semicolons, and
clears the program whenever any syntax error remains. Recovery improves
diagnostics without weakening acceptance rules.

### 11.3 Avoiding cascading semantic errors

An undeclared identifier has no reliable type. Treating it as `int` or `bool`
could create additional misleading errors. The `Unknown` type stops that cascade
while allowing independent statements to be checked.

### 11.4 Integrating independently developed modules

During development, lexer, parser, AST, semantic, and TAC revisions temporarily
used incompatible interfaces. A non-fast-forward GitHub update also revealed
that newer remote work had arrived. Rather than force-pushing, the remote history
was inspected and matching revisions were selected. The complete tree was then
assembled, configured from scratch, compiled, and tested before restoration.
This preserved the SQL prototype on `sql-prototype` while converting `main` to
the manual-specified compiler.

### 11.5 Making outputs reproducible

TAC temporaries and labels are allocated deterministically. Each golden program
has an expected output file, and the test runner compares the complete actual
output rather than checking only whether compilation returns success. This makes
changes in expression order, labels, jumps, and diagnostics visible.

## 12. Testing

### 12.1 Method

The test strategy has three layers:

1. Unit tests cover AST ownership and printing, tokenization, parsing and error
   recovery, semantic rules, and TAC construction.
2. Golden-file tests drive every `tests/programs/*.mc` file through the complete
   pipeline and compare it byte-for-byte with `tests/expected/*.out`.
3. CLI tests cover default compilation, token output, AST output, help, standard
   input, invalid syntax, and invalid semantics.

The clean verification environment was GNU C++ 12.2.0, Flex 2.6.4, Bison 3.8.2,
CMake 4.4.5, and Linux. Windows instructions use PowerShell, MinGW-w64,
`win_flex`, and `win_bison`.

Commands:

```sh
cmake -S compiler -B build/compiler -DOPTIQ_BUILD_TESTS=ON
cmake --build build/compiler --parallel
ctest --test-dir build/compiler --output-on-failure
```

Actual result on the restored GitHub `main` revision:

```text
100% tests passed, 0 tests failed out of 13
```

The build completed with zero compiler warnings, and Bison reported zero grammar
conflicts.

### 12.2 Golden program coverage

| Program | Purpose | Actual result |
| --- | --- | --- |
| `01-valid-full.mc` | Declarations, assignments, loop, if-else, and print | TAC exactly matches expected output |
| `02-valid-float.mc` | Float operations and `int` to `float` widening | Accepted; TAC matches expected output |
| `03-error-lexical.mc` | Invalid lexical element | Rejected; diagnostic matches expected output |
| `04-error-syntax.mc` | Malformed statement syntax | Rejected; diagnostic matches expected output |
| `05-error-semantic-undeclared.mc` | Undeclared assignment target | Rejected with undeclared-variable diagnostic |
| `06-error-semantic-redeclaration.mc` | Duplicate declaration in one scope | Rejected with redeclaration diagnostic |
| `07-error-semantic-scope.mc` | Use of a name after its block ends | Rejected with scope/declaration diagnostic |
| `08-error-semantic-type-mismatch.mc` | Invalid initializer type | Rejected with type-mismatch diagnostic |
| `09-error-semantic-invalid-assignment.mc` | Incompatible assignment | Rejected with assignment diagnostic |
| `10-error-semantic-invalid-expression.mc` | Invalid operand types | Rejected with operator diagnostic |
| `11-error-semantic-condition.mc` | Non-Boolean control condition | Rejected with condition diagnostic |

All eleven actual outputs match their paired expected files. The CTest suite
additionally passes `ast_tests`, `lexer_tests`, `parser_tests`, `semantic_tests`,
`tac_tests`, `example_tests`, and seven CLI tests. Detailed results are recorded
in [verification.md](verification.md).

### 12.3 Reproducing the demonstration

Build the project, then run:

```sh
./build/compiler/optiq_cli examples/06-manual-sample.mc
./build/compiler/optiq_cli --ast examples/03-if-else.mc
./build/compiler/optiq_cli tests/programs/03-error-lexical.mc
./build/compiler/optiq_cli tests/programs/04-error-syntax.mc
./build/compiler/optiq_cli tests/programs/05-error-semantic-undeclared.mc
```

The first command demonstrates valid TAC; the second demonstrates the AST; the
remaining commands demonstrate the three required error classes. Expected error
exit codes are `2` for lexical/syntax errors and `3` for semantic errors.

## 13. Conclusion

Optiq demonstrates the complete required front-end pipeline as one coherent
system. Flex converts characters into located tokens; Bison enforces a formal,
conflict-free grammar and constructs the AST; semantic analysis and the symbol
table enforce context-sensitive rules; and the generator emits deterministic
TAC for expressions and structured control flow.

The project shows why each phase depends on a clear interface with the preceding
phase. It also demonstrates that diagnostics, scope handling, type propagation,
and output verification are as important as accepting a valid sample. Automated
unit, golden-file, and CLI tests provide reproducible evidence for both accepted
and rejected programs.

Future work could add the manual's optional bonus features, but the current
priority remains understanding and demonstrating the required core rather than
expanding into an out-of-scope backend.

## 14. References

1. Metropolitan University, Department of Computer Science and Engineering.
   *Compiler Construction Lab Project Manual*, especially Sections 4, 5, 10–12,
   and 15.
2. Flex Project. *Flex: Fast Lexical Analyzer Generator Manual*.
   https://westes.github.io/flex/manual/
3. Free Software Foundation. *Bison: GNU Parser Generator Manual*.
   https://www.gnu.org/software/bison/manual/
4. CMake. *CMake Reference Documentation*.
   https://cmake.org/cmake/help/latest/
5. ISO C++ Foundation. *C++17 Standard and Language References*.
   https://isocpp.org/std/the-standard
6. Optiq source, tests, commit history, and verification record.
   https://github.com/Shadow-Taslim/OptiqComplier
