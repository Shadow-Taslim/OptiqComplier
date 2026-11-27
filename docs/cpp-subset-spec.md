# OptiQ C/C++ Common Subset Specification

## 1. Status and purpose

OptiQ's primary source language is an explicitly limited common subset of C and
C++. **Weeks 1–3 implement lexical analysis, syntax analysis, and a preliminary
AST.** Successful parsing proves only that source conforms to the grammar in this
document. It does not prove semantic validity, type safety, name resolution, or
executability.

The source character set is plain text. Keywords are lowercase and
case-sensitive. Source locations are one-based; a tab advances the column by one.
No complete C or C++ implementation is claimed.

## 2. Lexical structure

### 2.1 Keywords and parser-reserved names

The Flex lexer classifies these as `KEYWORD` tokens:

```text
int float double char bool void
if else while for do return break continue
true false
```

The parser recognizes `break` and `continue` tokens but rejects them because
Week 3 does not define their statements. The parser additionally reserves the
identifier spellings `using`, `namespace`, `std`, `cout`, `cin`, and `endl` for
namespace and stream syntax. They are not available as ordinary identifiers in
this subset.

### 2.2 Identifiers

```text
identifier ::= (letter | "_") (letter | digit | "_")*
```

Examples: `x`, `total`, `studentName`, `_value`, and `counter123`.

### 2.3 Integer and floating-point literals

Decimal integer literals contain one or more digits: `0`, `42`, `1000`.
Hexadecimal, octal, binary, exponent, and integer-suffix forms are unsupported.
A leading minus is a separate unary operator.

```text
floating_literal ::= digit+ "." digit+ ("f" | "F")?
```

Examples: `3.14`, `0.5`, `25.50`, `1.25f`, and `2.0F`. Exponent notation, `.5`,
`1.`, and long-double suffixes are rejected. Forms such as `123abc`, `12.3.4`,
`1e3`, and `1.0ff` produce malformed-numeric-literal diagnostics.

### 2.4 Character and string literals

Simple character literals (`'a'`, `'Z'`) and string literals (`"Hello World"`)
are supported. A character literal contains exactly one character or one valid
escape. Supported escapes are:

```text
\n  \t  \\  \"  \'
```

Other escapes, raw newlines in literals, empty/multi-character character
literals, and unterminated literals are diagnosed.

### 2.5 Operators and delimiters

| Category | Operators |
| --- | --- |
| Arithmetic | `+` `-` `*` `/` `%` |
| Relational/equality | `<` `>` `<=` `>=` `==` `!=` |
| Logical | `&&` `||` `!` |
| Assignment | `=` `+=` `-=` `*=` `/=` |
| Increment/decrement | `++` `--` |
| Bitwise | `&` `|` `^` `~` |
| Stream insertion/extraction | `<<` `>>` |
| Tokenized but not parsed | `->` `.` `[` `]` `:` |

Delimiters used by the grammar are `( ) { } ; ,`. `::` is accepted only in the
documented qualified stream/manipulator forms. Flex longest-match behavior keeps
`==`, `+=`, `++`, `<<`, and `::` as single tokens.

### 2.6 Comments and whitespace

`//` starts a line comment. `/* ... */` is a non-nesting block comment. Comments
and whitespace are discarded while their characters still contribute to source
locations. An unterminated block comment is an error.

### 2.7 Limited preprocessor recognition

At the start of a line, optionally after spaces or tabs, these forms produce a
single `PREPROCESSOR` token and a corresponding AST node:

```cpp
#include <iostream>
#include <vector>
#include "local.hpp"
#define MAX 100
#define EMPTY
```

`#include` requires an angle-bracket or quoted header. `#define` requires a valid
identifier. Other directives are rejected. There is **no macro expansion, file
inclusion, conditional compilation, or full preprocessor**.

## 3. Program and declaration grammar

The following notation summarizes the implemented Bison grammar; it is not a
complete ISO C/C++ grammar.

```text
translation-unit       ::= external-declaration*
external-declaration   ::= preprocessor-directive
                         | "using" "namespace" "std" ";"
                         | variable-declaration
                         | function-prototype
                         | function-definition

variable-declaration   ::= value-type init-declarator ("," init-declarator)* ";"
init-declarator        ::= identifier ("=" assignment-expression)?
value-type             ::= "int" | "float" | "double" | "char" | "bool"
return-type            ::= value-type | "void"

function-prototype     ::= return-type identifier "(" parameters? ")" ";"
function-definition    ::= return-type identifier "(" parameters? ")" block
parameters             ::= parameter ("," parameter)*
parameter              ::= value-type identifier
```

An empty translation unit is valid. Global and local variable declarations are
supported. Multiple declarators are supported in ordinary declarations. A `for`
initializer permits only one declarator. `void` is accepted for function return
types and rejected for variables and parameters with an explicit diagnostic.
Function overloading, templates, default arguments, and unnamed parameters are
outside the subset.

## 4. Statements

```text
statement              ::= block
                         | expression ";"
                         | ";"
                         | if-statement
                         | while-statement
                         | for-statement
                         | do-while-statement
                         | "return" expression? ";"
                         | stream-output ";"
                         | stream-input ";"

block                  ::= "{" (variable-declaration | statement)* "}"
if-statement           ::= "if" "(" expression ")" statement
                         | "if" "(" expression ")" statement "else" statement
while-statement        ::= "while" "(" expression ")" statement
do-while-statement     ::= "do" statement "while" "(" expression ")" ";"
for-statement          ::= "for" "(" for-init condition? ";" expression? ")" statement
for-init               ::= ";"
                         | value-type init-declarator ";"
                         | expression ";"
condition              ::= expression
```

The `for-init` notation includes its own first semicolon; `condition` is followed
by the second. Empty initialization, condition, and update clauses are accepted.
An `else` binds to the nearest unmatched `if`. Blocks may be nested and may mix
supported declarations and statements.

## 5. Expressions, precedence, and associativity

Expressions include integer, floating, character, string, and boolean literals;
identifiers; parentheses; unary and binary operators; assignment; calls; and
prefix/postfix increment/decrement. There is no comma operator.

From lowest to highest precedence:

| Level | Operators | Associativity |
| --- | --- | --- |
| 1 | `=`, `+=`, `-=`, `*=`, `/=` | Right; target must be an identifier |
| 2 | `||` | Left |
| 3 | `&&` | Left |
| 4 | `|` | Left |
| 5 | `^` | Left |
| 6 | `&` | Left |
| 7 | `==`, `!=` | Left |
| 8 | `<`, `>`, `<=`, `>=` | Left |
| 9 | `+`, `-` | Left |
| 10 | `*`, `/`, `%` | Left |
| 11 | Prefix `++`, `--`, `+`, `-`, `!`, `~` | Right |
| 12 | Calls, postfix `++`, postfix `--` | Left |
| 13 | Literals, identifiers, parenthesized expressions | Primary |

Therefore `10 + 5 * 2` has `+` at the AST root with `5 * 2` as its right child,
and `x = y = 1` nests the second assignment in the first assignment's value.
General shift expressions are deliberately unsupported: `<<` and `>>` belong to
stream statements in Week 3. Operand type and lvalue validation beyond the
identifier assignment target are deferred to semantic analysis.

## 6. Function calls

```text
postfix-expression     ::= primary-expression
                         | postfix-expression "(" arguments? ")"
                         | postfix-expression "++"
                         | postfix-expression "--"
arguments              ::= expression ("," expression)*
```

Empty and populated argument lists are recognized. Week 3 checks call syntax
only; it does not check whether a callee exists, is callable, or receives the
correct argument types.

## 7. C++ stream syntax

```text
stream-output          ::= output-stream "<<" stream-element
                           ("<<" stream-element)*
output-stream          ::= "cout" | "std" "::" "cout"
stream-element         ::= assignment-expression | "endl" | "std" "::" "endl"

stream-input           ::= input-stream ">>" identifier (">>" identifier)*
input-stream           ::= "cin" | "std" "::" "cin"
```

Examples:

```cpp
cout << "Hello World" << endl;
std::cout << "Hello World" << std::endl;
cin >> x;
std::cin >> x >> y;
```

These forms build `StreamOutputNode` or `StreamInputNode` objects. No standard
library implementation, formatting behavior, or runtime I/O is generated.

## 8. Preliminary AST

The AST is separate from the grammar and uses `std::unique_ptr` ownership. Every
node stores a `SourceLocation`. Implemented categories are:

- `ProgramNode`, `FunctionDeclarationNode`, `FunctionDefinitionNode`, and
  `ParameterNode`.
- `VariableDeclarationNode`, `BlockStatementNode`, `ExpressionStatementNode`,
  `EmptyStatementNode`, and `ErrorStatementNode`.
- `IfStatementNode`, `WhileStatementNode`, `ForStatementNode`,
  `DoWhileStatementNode`, and `ReturnStatementNode`.
- `IdentifierNode`, `LiteralNode`, `UnaryExpressionNode`,
  `BinaryExpressionNode`, `AssignmentExpressionNode`, and `FunctionCallNode`.
- `StreamOutputNode`, `StreamInputNode`, `PreprocessorNode`, and
  `UsingNamespaceNode`.

`optiq_cpp --ast` prints a stable indented representation. Recovered error nodes
may exist internally during parsing, but failed parses do not expose a partial
AST through `ParseResult` or the CLI.

## 9. Diagnostics, recovery, and CLI result

The lexer reports invalid characters, malformed numeric literals, invalid
escapes, malformed literals, unterminated comments, and unsupported directives.
The Bison parser reports unexpected tokens with one-based line/column locations
and expected-token information when Bison can provide a concise expected set.
The parser uses `error SEMICOLON` plus `yyerrok` to recover at selected statement
boundaries. Recovery never converts an invalid source into a successful result.

```sh
optiq_cpp --tokens program.cpp
optiq_cpp --parse program.cpp
optiq_cpp --ast program.cpp
```

The default is `--tokens`. All modes accept standard input when no file is given.
Exit code `0` means the requested operation succeeded; `2` means lexical or
syntax diagnostics were emitted; `1` means command-line, file-I/O, or internal
failure. If lexical analysis fails, parsing is not attempted.

## 10. Out of scope after Week 3

Complete C or C++, pointers, arrays, structs/unions/enums, classes, inheritance,
templates, overloading, exceptions, references, casts, switch statements,
`break`/`continue`, general member/namespace qualification, general shift
expressions, full preprocessing, semantic analysis, type checking, name
resolution, TAC, optimization, object code, assembly, linking, and executable
generation are outside this phase.
