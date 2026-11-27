# OptiQ C/C++ Common Subset Specification

## 1. Status and purpose

OptiQ's primary source language is an explicitly limited common subset of C and
C++. **Weeks 1–2 implement lexical analysis only.** Recognizing a token does
not mean the surrounding declaration, statement, expression, or program has
been parsed, type-checked, or compiled to executable code.

The source character set is plain text. Keywords are lowercase and
case-sensitive. Source locations are one-based; a tab advances the column by
one.

## 2. Lexical structure

### 2.1 Keywords

```text
int float double char bool void
if else while for do return break continue
true false
```

`true` and `false` are emitted as `KEYWORD` tokens. No additional keyword is
recognized implicitly; `Int`, for example, is an identifier.

### 2.2 Identifiers

```text
identifier ::= (letter | "_") (letter | digit | "_")*
```

Examples: `x`, `total`, `studentName`, `_value`, and `counter123`.

### 2.3 Integer literals

Decimal integer literals contain one or more digits: `0`, `42`, `1000`.
Hexadecimal, octal, binary, exponent, and integer-suffix forms are outside the
initial subset. A leading minus is a separate `MINUS` token.

### 2.4 Floating-point literals

A floating-point literal has digits on both sides of its decimal point:

```text
floating_literal ::= digit+ "." digit+ ("f" | "F")?
```

Examples: `3.14`, `0.5`, `25.50`, `1.25f`, and `2.0F`. Unsuffixed values are
double-compatible; `f`/`F` marks float-compatible syntax. Exponent notation,
`.5`, `1.`, and `long double` suffixes are not accepted. Forms such as `123abc`,
`12.3.4`, `1e3`, and `1.0ff` produce a malformed-numeric-literal diagnostic.

### 2.5 Character and string literals

Simple character literals (`'a'`, `'Z'`) and string literals (`"Hello World"`)
are recognized. A character literal must contain exactly one character or one
valid escape sequence.

The supported escapes are:

```text
\n  \t  \\  \"  \'
```

Other escapes are diagnosed. Raw newlines and end-of-input terminate an
unterminated literal with a diagnostic; they are not accepted as literal
contents.

### 2.6 Operators

| Category | Operators |
| --- | --- |
| Arithmetic | `+` `-` `*` `/` `%` |
| Relational | `<` `>` `<=` `>=` `==` `!=` |
| Logical | `&&` `||` `!` |
| Assignment | `=` `+=` `-=` `*=` `/=` |
| Increment/decrement | `++` `--` |
| Bitwise/shift | `&` `|` `^` `~` `<<` `>>` |
| Other | `->` `.` `::` |

Flex longest-match behavior is required: `==`, `+=`, `++`, `<<`, and `::` are
single tokens, not sequences of shorter tokens. `<<` and `>>` use the token
names `SHIFT_LEFT` and `SHIFT_RIGHT`, including when they occur in stream I/O.

### 2.7 Delimiters

```text
( ) { } [ ] ; , :
```

### 2.8 Comments and whitespace

`//` starts a line comment. `/* ... */` is a non-nesting block comment.
Comments and whitespace are discarded while their characters still contribute
to source locations. An unterminated block comment is an error.

### 2.9 Limited preprocessor recognition

At the start of a line, optionally after spaces or tabs, these forms produce a
single `PREPROCESSOR` token:

```cpp
#include <iostream>
#include "local.hpp"
#define MAX 100
#define EMPTY
```

`#include` requires an angle-bracket or quoted header. `#define` requires a
valid identifier; its optional replacement text is retained only as part of the
token lexeme. Other directives, including `#pragma` and conditional directives,
produce an explicit unsupported-directive diagnostic.

There is **no macro expansion, file inclusion, conditional compilation, or
full C/C++ preprocessor**.

## 3. Language foundations for later phases

The subset specification reserves the following constructs for the future
Bison parser:

- Types: `int`, `float`, `double`, `char`, `bool`, and `void` where appropriate.
- Variable declarations, initialization, and assignment.
- Expression statements and compound `{ ... }` blocks.
- Basic C-style I/O token sequences and C++ `cin`/`cout` stream token sequences.

These constructs are **not parsed or executable yet**. In particular,
`std::cout << "text"` currently demonstrates tokenization of `std`, `::`,
`cout`, `<<`, and a string literal—not stream-I/O compilation.

## 4. Diagnostics and CLI result

The lexer reports invalid characters, malformed numeric literals, invalid
escapes, empty or multi-character character literals, unterminated strings,
unterminated character literals, unterminated block comments, and unsupported
or malformed preprocessor directives. Diagnostics include line and column.

`optiq_cpp` returns `0` for successful tokenization, `2` when one or more
lexical diagnostics are emitted, and `1` for command-line, file-I/O, or
internal errors.

## 5. Out of scope in Weeks 1–2

Complete C or C++, a parser, AST, semantic analysis, type checking, macro
expansion, TAC, optimization, object code, assembly, linking, and executable
generation are outside this phase. Later phases must extend this document
before extending the recognized language.
