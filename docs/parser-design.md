# C/C++ Parser Design

## 1. Scope

Week 3 adds a GNU Bison syntax analyzer and preliminary AST to the existing C/C++
frontend. The parser recognizes only [cpp-subset-spec.md](cpp-subset-spec.md).
It performs no name resolution, type checking, TAC generation, optimization, or
code generation.

## 2. Processing pipeline

```text
Source text
    │
    ▼
Existing reentrant Flex scanner
    │  LexResult: vector<Token> + lexical diagnostics
    ▼
Token-vector adapter in parser.cpp
    │  TokenType/lexeme → Bison symbol_type + location
    ▼
Reentrant Bison C++ LALR(1) parser
    │  grammar actions with std::unique_ptr
    ▼
Preliminary C/C++ AST
    │
    ├── --parse: status summary
    └── --ast: indented AST printer
```

Lexical errors stop the pipeline before Bison starts. Syntax errors leave
`ParseResult::program` null even when statement-level recovery allowed Bison to
inspect later input.

## 3. Flex/Bison interface

The Week 2 Flex rules are unchanged. `tokenize()` still owns the complete Flex
lifecycle and returns independent token data. The parser adapter does not invoke
Flex incrementally and does not duplicate scanner state.

The adapter in `compiler/cpp-frontend/src/parser.cpp` maps the existing
`TokenType` values to Bison token constructors. Literal, identifier, and
preprocessor lexemes are carried as `std::string` semantic values. Keywords are
classified by their existing lexemes. The parser-only names `using`,
`namespace`, `std`, `cout`, `cin`, and `endl` receive distinct Bison symbols.

Bison locations are reconstructed from each token's one-based start location and
lexeme. End-of-input receives a location computed from the complete source. AST
nodes retain the start line and column; the original byte offset remains
available in lexer tokens but is not required by the printer.

Relevant files:

| File | Responsibility |
| --- | --- |
| `src/parser.y` | Grammar, precedence hierarchy, AST actions, and recovery production |
| `src/parser.cpp` | Token adapter, public `parse()` driver, EOF locations, error callback |
| `src/parser_context.hpp` | Owned token stream, cursor, diagnostics, and result program |
| `include/optiq/cpp/parser.hpp` | Public parser result and API; no generated Bison dependency |

Generated `cpp_parser.cpp`, `cpp_parser.hpp`, and location sources stay in the
CMake build directory.

## 4. Bison configuration

The grammar uses:

```text
%skeleton "lalr1.cc"
%define api.namespace {optiq::cpp::syntax}
%define api.parser.class {Parser}
%define api.value.type variant
%define api.token.constructor
%define parse.error detailed
%define parse.lac full
%locations
```

`api.value.type variant` provides type-safe C++ semantic values and automatic
destruction of `std::unique_ptr` values on normal reductions and error cleanup.
Explicit parse/lex parameters carry `ParserContext`; no mutable global parser or
scanner result is used. The verified Bison run reports no shift/reduce or
reduce/reduce conflicts.

## 5. Grammar organization

Expressions use separate nonterminal levels rather than a single ambiguous rule.
This makes precedence and left/right associativity visible in the grammar and
directly testable in the AST. The levels, from lowest to highest, are assignment,
logical OR, logical AND, bitwise OR/XOR/AND, equality, relational, additive,
multiplicative, prefix unary, postfix, and primary.

Assignments recurse on the right and accept an identifier target. Calls and
postfix increment/decrement recurse on the left. General shifts are not mixed
into this hierarchy because their Week 3 role is stream I/O.

Dangling `else` is resolved with an explicit lower-precedence marker on the
no-`else` production and higher precedence on `else`. The selected behavior is
the conventional nearest-`if` binding.

`void` is a valid return type but not a variable or parameter type. Those
contexts add explicit located syntax diagnostics instead of silently accepting
invalid subset declarations.

## 6. AST ownership and representation

`Node` is the polymorphic base and stores `NodeKind` plus `SourceLocation`.
`Expression` and `Statement` provide broad categories. Concrete nodes contain
public, typed data needed by the Week 4 semantic analyzer. Children use:

```cpp
using NodePtr = std::unique_ptr<Node>;
using NodeList = std::vector<NodePtr>;
```

There is no raw-pointer ownership, manual deletion in grammar actions, or sharing
with the academic AST. Multiple declarators become separate
`VariableDeclarationNode` objects. A function definition owns its parameter
nodes and body. Optional return expressions, `else` branches, and `for` clauses
use null `NodePtr` values.

`ast_printer.cpp` is a read-only visitor-style printer based on `NodeKind`. It
prints stable labels, operators, names, literal categories/values, child labels,
and `@line:column` locations. AST declarations, construction, and printing are
therefore separate responsibilities.

## 7. Diagnostics and recovery

`Parser::error()` removes Bison's redundant `syntax error, ` prefix and records
the remaining detailed message in `ParserContext`. The CLI adds the source name
and `line:column` before printing.

The recovery production is:

```bison
statement:
    error SEMICOLON {
        yyerrok;
        // Build an internal ErrorStatementNode.
    }
;
```

This synchronizes at a statement terminator and permits later statements to be
checked without treating recovery as success. Errors involving missing braces or
unterminated top-level declarations may stop at end-of-input. Bison's usual
error-state suppression limits cascades; the recovery test verifies one
diagnostic for a bad declaration followed by valid statements.

## 8. CLI behavior

- `--tokens`: unchanged Week 2 token table and default mode.
- `--parse`: tokenize, parse, and print lexer/parser/AST status.
- `--ast`: print the generated tree only when parsing succeeds.
- No option: same as `--tokens`.
- File omitted: read source from standard input.

The three operation modes are mutually exclusive. Exit codes are 0 for success,
2 for lexical/syntax failure, and 1 for command/I/O/internal errors.

## 9. Testing and extension contract

`parser_tests.cpp` covers all requested positive and negative categories and
inspects AST shape for multiplication precedence, right-associative assignment,
node locations, type restrictions, and semicolon recovery. CMake CLI tests check
actual process exit codes and output. All Week 2 tests remain in the same CTest
project, and the academic project still builds separately.

Week 4 should consume `ParseResult::program` only after `success()` is true. It
can add symbol/type passes without changing the Flex scanner or taking ownership
away from the AST. Week 5 must remain downstream of that semantic pass.
