# OptiQ Week 3 Completion Report

## 1. Scope and repository state

Week 3 implements the C/C++ subset Bison parser, preliminary AST, AST printer,
located diagnostics, statement-level recovery, CLI integration, examples,
documentation, and automated tests. It does not implement semantic analysis,
TAC, optimization, preprocessing, or code generation.

- **Base branch:** `main`
- **Base revision:** `39473c5f9dbaa8638c3b62f1071bc6d9167cfef5`
- **Publication:** staged normal commits on `main`
- **Branch changes:** no deletion, force-push, or default-branch change
- **Academic baseline:** not modified
- **Flex rule changes:** none; the existing token stream is reused

## 2. Delivered implementation

1. Complete Bison C++ grammar in `compiler/cpp-frontend/src/parser.y`.
2. Token-vector Flex/Bison adapter and public parser driver in `parser.cpp`.
3. Typed AST with `unique_ptr` ownership and source locations in `ast.hpp`.
4. Stable indented AST printer in `ast_printer.cpp`.
5. `--tokens`, `--parse`, and `--ast` CLI modes.
6. Detailed Bison diagnostics with source names and one-based locations.
7. `error SEMICOLON`/`yyerrok` statement synchronization.
8. Twenty-three positive and twelve negative parser tests.
9. Five structural AST/type/recovery tests and three new CLI tests.
10. Full parser and syntax-error sample programs.
11. Updated specification, architecture, roadmap, README, and verification.
12. This per-test report and build/test instructions.

### Changed files

**Modified (7):**

- `README.md`
- `compiler/cpp-frontend/CMakeLists.txt`
- `compiler/cpp-frontend/src/cli.cpp`
- `docs/architecture.md`
- `docs/cpp-subset-spec.md`
- `docs/development-roadmap.md`
- `docs/verification.md`

**Added (16):**

- `compiler/cpp-frontend/include/optiq/cpp/ast.hpp`
- `compiler/cpp-frontend/include/optiq/cpp/ast_printer.hpp`
- `compiler/cpp-frontend/include/optiq/cpp/parser.hpp`
- `compiler/cpp-frontend/src/ast.cpp`
- `compiler/cpp-frontend/src/ast_printer.cpp`
- `compiler/cpp-frontend/src/parser.cpp`
- `compiler/cpp-frontend/src/parser.y`
- `compiler/cpp-frontend/src/parser_context.hpp`
- `docs/parser-design.md`
- `docs/week3-completion.md`
- `examples/cpp-subset/03-parser-full.cpp`
- `examples/cpp-subset/04-syntax-errors.cpp`
- `tests/cpp-frontend/cli_ast_test.cmake`
- `tests/cpp-frontend/cli_parse_test.cmake`
- `tests/cpp-frontend/cli_syntax_error_test.cmake`
- `tests/cpp-frontend/parser_tests.cpp`

No files were deleted. No academic compiler source, grammar, test, example, or
language-specification file was changed.

## 3. Build and test instructions

Dependencies: CMake 3.20+, C++17, Flex 2.6+, and Bison 3.8+.

```sh
make cpp
make test-cpp
make test-mini
make test
```

Standalone commands:

```sh
cmake -S compiler/cpp-frontend -B build/cpp-frontend -DOPTIQ_CPP_BUILD_TESTS=ON
cmake --build build/cpp-frontend --parallel
ctest --test-dir build/cpp-frontend --output-on-failure
```

CLI examples:

```sh
./build/cpp-frontend/optiq_cpp --tokens examples/cpp-subset/01-full.cpp
./build/cpp-frontend/optiq_cpp --parse examples/cpp-subset/03-parser-full.cpp
./build/cpp-frontend/optiq_cpp --ast examples/cpp-subset/03-parser-full.cpp
./build/cpp-frontend/optiq_cpp --parse examples/cpp-subset/04-syntax-errors.cpp
```

## 4. Executed verification summary

Environment: GNU C++ 12.2.0, Flex 2.6.4, Bison 3.8.2, CMake 3.25.1, Linux.

| Suite/check | Expected | Actual | Status |
| --- | --- | --- | --- |
| Baseline academic | Existing 13 tests | 13/13 passed before changes | PASS |
| Baseline C/C++ | Existing 21 tests | 21/21 passed before changes | PASS |
| Final academic | No regression | `100% tests passed, 0 tests failed out of 13` | PASS |
| Final C/C++ | All 64 tests | `100% tests passed, 0 tests failed out of 64` | PASS |
| Bison generation | No conflicts | No shift/reduce or reduce/reduce warnings | PASS |
| Valid parser sample | Exit 0 and AST | Exit 0; lexer/parser success; AST generated | PASS |
| Invalid parser sample | Exit 2 and diagnostic | Exit 2; line 2, column 17; no AST | PASS |
| Native Windows | Not required for this executed record | Not executed | NOT VERIFIED |

The final C/C++ count consists of 18 lexer tests, 40 parser/structural tests,
and six CLI tests. All 21 Week 2 C/C++ tests remain registered and passing.

## 5. Positive parser tests

Every row below was executed through `optiq_cpp_parser_tests`. “AST generated”
means a non-null `ProgramNode` with no lexical or syntax diagnostics.

| Test name | Source input | Expected parser result | Actual parser result | Status |
| --- | --- | --- | --- | --- |
| `positive_empty_translation_unit` | Empty source | Accept; empty Program AST | Parser SUCCESS; AST generated | PASS |
| `positive_main_function` | `int main() { return 0; }` | Accept main | Parser SUCCESS; AST generated | PASS |
| `positive_variable_declarations` | `int main() { int x, y, z; float price; char grade; bool active; return 0; }` | Accept multiple/single declarations | Parser SUCCESS; AST generated | PASS |
| `positive_variable_initialization` | `int main() { double total = 25.5; char grade = 'A'; bool active = true; return 0; }` | Accept initializers | Parser SUCCESS; AST generated | PASS |
| `positive_arithmetic_expressions` | `int main() { int r = (10 + 20) * 2 - 3 / 1 % 2; return r; }` | Accept arithmetic | Parser SUCCESS; AST generated | PASS |
| `positive_relational_expressions` | `int main() { bool r = 1 < 2 && 3 >= 2 && 4 != 5; return 0; }` | Accept relational/equality syntax | Parser SUCCESS; AST generated | PASS |
| `positive_logical_expressions` | `int main() { bool r = !false || true && false; return 0; }` | Accept logical syntax | Parser SUCCESS; AST generated | PASS |
| `positive_nested_expressions` | `int main() { int r = ((10 + 5) * (2 + 1)); return r; }` | Accept nested parentheses | Parser SUCCESS; AST generated | PASS |
| `positive_if_statement` | `int main() { int x = 11; if (x > 10) { x = x - 1; } return x; }` | Accept if | Parser SUCCESS; AST generated | PASS |
| `positive_if_else_statement` | `int main() { int x = 0; if (x) x = 1; else x = 2; return x; }` | Accept if/else | Parser SUCCESS; AST generated | PASS |
| `positive_while_loop` | `int main() { int x = 3; while (x > 0) { x--; } return x; }` | Accept while | Parser SUCCESS; AST generated | PASS |
| `positive_for_loop` | `int main() { int x = 0; for (int i = 0; i < 10; i++) { x += i; } return x; }` | Accept for | Parser SUCCESS; AST generated | PASS |
| `positive_do_while_loop` | `int main() { int x = 3; do { x--; } while (x > 0); return x; }` | Accept do/while | Parser SUCCESS; AST generated | PASS |
| `positive_function_definition` | `int add(int a, int b); int add(int a, int b) { return a + b; }` | Accept prototype and definition | Parser SUCCESS; AST generated | PASS |
| `positive_function_call` | `int add(int a, int b) { return a + b; } int main() { int r = add(10, 20); int z = add(); return r; }` | Accept populated/empty calls | Parser SUCCESS; AST generated | PASS |
| `positive_return_statement` | `void done() { return; } int main() { return 0; }` | Accept void/valued returns | Parser SUCCESS; AST generated | PASS |
| `positive_nested_blocks` | `int main() { int x = 10; { int y = 20; x = x + y; } return x; }` | Accept nested block | Parser SUCCESS; AST generated | PASS |
| `positive_increment_decrement` | `int main() { int i = 0; i++; ++i; i--; --i; return i; }` | Accept prefix/postfix forms | Parser SUCCESS; AST generated | PASS |
| `positive_cout_syntax` | `int main() { cout << "Hello World" << endl; return 0; }` | Accept output chain | Parser SUCCESS; AST generated | PASS |
| `positive_cin_syntax` | `int main() { int x; cin >> x; return 0; }` | Accept input | Parser SUCCESS; AST generated | PASS |
| `positive_qualified_stream_syntax` | `int main() { int x; std::cin >> x; std::cout << x << std::endl; return 0; }` | Accept qualified streams | Parser SUCCESS; AST generated | PASS |
| `positive_include_directives` | `#include <iostream>\n#include <vector>\nint main() { return 0; }` | Accept includes | Parser SUCCESS; AST generated | PASS |
| `positive_namespace_declaration` | `#include <iostream>\nusing namespace std;\nint main() { cout << endl; return 0; }` | Accept namespace syntax | Parser SUCCESS; AST generated | PASS |

## 6. Negative parser tests

Each fixture reached Bison without lexical errors, produced at least one located
syntax diagnostic, returned an unsuccessful `ParseResult`, and exposed no partial
AST.

| Test name | Source input | Expected parser result | Actual parser result | Status |
| --- | --- | --- | --- | --- |
| `negative_missing_semicolon` | `int main() { return 0 }` | Reject | Parser FAILED; located syntax diagnostic | PASS |
| `negative_missing_closing_brace` | `int main() { return 0;` | Reject | Parser FAILED; located syntax diagnostic | PASS |
| `negative_missing_opening_parenthesis` | `int main) { return 0; }` | Reject | Parser FAILED; located syntax diagnostic | PASS |
| `negative_invalid_expression` | `int main() { int x = 10 + * 2; return 0; }` | Reject | Parser FAILED; located syntax diagnostic | PASS |
| `negative_incomplete_assignment` | `int main() { int x; x = ; return 0; }` | Reject | Parser FAILED; located syntax diagnostic | PASS |
| `negative_invalid_function_declaration` | `int main( { return 0; }` | Reject | Parser FAILED; located syntax diagnostic | PASS |
| `negative_incorrect_parameter_syntax` | `int add(int a, b) { return a; }` | Reject | Parser FAILED; located syntax diagnostic | PASS |
| `negative_invalid_for_loop` | `int main() { for (int i = 0 i < 10; i++) { i++; } return 0; }` | Reject | Parser FAILED; located syntax diagnostic | PASS |
| `negative_invalid_stream_insertion` | `int main() { cout << ; return 0; }` | Reject | Parser FAILED; located syntax diagnostic | PASS |
| `negative_unexpected_token` | `int main() { int x = 1 ] ; return 0; }` | Reject | Parser FAILED; located syntax diagnostic | PASS |
| `negative_malformed_nested_expression` | `int main() { int x = (10 + 5 * 2; return 0; }` | Reject | Parser FAILED; located syntax diagnostic | PASS |
| `negative_invalid_control_flow` | `int main() { int x = 1; if x > 10) { x++; } return 0; }` | Reject | Parser FAILED; located syntax diagnostic | PASS |

## 7. Structural parser and AST tests

| Test name | Source input | Expected result | Actual result | Status |
| --- | --- | --- | --- | --- |
| `operator_precedence` | `int main() { int result = 10 + 5 * 2; return result; }` | `+` root; `*` right child | Verified exact AST shape | PASS |
| `assignment_associativity` | `int main() { int x; int y; x = y = 1; return 0; }` | Right-associative assignment | Verified nested right assignment | PASS |
| `ast_source_locations` | Include, main, and `int value = 42` on separate lines | Exact node locations and printer output | Program 1:1; main 2:1; declarator 3:7; literal 3:15 | PASS |
| `error_recovery` | `int main() { int x = ; x = 1; return 0; }` | Recover at semicolon without success | One diagnostic; no partial AST | PASS |
| `type_restrictions` | `void done(){return;}` plus void variable/parameter fixtures | Accept only void return | Return accepted; variable/parameter rejected | PASS |

## 8. New CLI tests

| Test name | Source input | Expected result | Actual result | Status |
| --- | --- | --- | --- | --- |
| `cpp_cli_parse_sample` | `examples/cpp-subset/03-parser-full.cpp` with `--parse` | Exit 0 and success summary | Exit 0; lexer/parser success; AST generated | PASS |
| `cpp_cli_ast_sample` | `03-parser-full.cpp` with `--ast` | Exit 0 and required node kinds | Exit 0; functions, loops, and streams printed | PASS |
| `cpp_cli_syntax_error` | `04-syntax-errors.cpp` with `--parse` | Exit 2 and located error | Exit 2; line 2, column 17; Parser FAILED | PASS |

The pre-existing token, lexical-error, and help CLI tests also passed. Their full
input/output records remain in [week2-completion.md](week2-completion.md).

## 9. Acceptance criteria

| Criterion | Evidence | Status |
| --- | --- | --- |
| Valid supported programs parse | 23 executed positive tests | PASS |
| Invalid programs rejected | 12 executed negative tests | PASS |
| Precedence and associativity | AST shape tests | PASS |
| Control flow, functions, calls, streams | Positive tests and full sample | PASS |
| AST generation and printing | Structural and `--ast` tests | PASS |
| Located diagnostics | Negative and CLI tests | PASS |
| Recovery without false success | Recovery test | PASS |
| Flex/Bison build | Clean executed build, no conflicts | PASS |
| Week 1–2 tests remain passing | All 21 prior C/C++ tests included | PASS |
| Academic compiler remains intact | 13/13 tests passed | PASS |

## 10. Deliberate limitations

No complete C/C++ language, actual preprocessing, semantic checks, name/type
resolution, TAC, optimization, backend, or executable generation is included.
General shifts, arrays, pointers, classes, templates, overloading, switch,
`break`, and `continue` remain unsupported. Native Windows verification remains
unexecuted. These limitations do not affect the executed Linux acceptance record.
