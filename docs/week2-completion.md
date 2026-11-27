# Week 2 Completion Report

## Outcome

The C/C++ frontend now provides a reentrant Flex lexer, explicit token model,
located diagnostics, file/stdin token-table CLI, examples, and automated tests.
It does not claim to parse or execute C/C++ programs.

Environment actually used: GNU C++ 12.2.0, Flex 2.6.4, CMake 3.25.1, Linux.

## Executed test report

Each result below comes from an executed CTest entry. The unit-test executable
also prints the test name, input description, expected result, actual assertion
count, and PASS/FAIL status.

| # | Test name | Input | Expected result | Actual result | Result |
| --- | --- | --- | --- | --- | --- |
| 1 | `cpp_keywords` | Sixteen documented keywords | 16 keyword tokens | 18 checks; all keywords recognized | PASS |
| 2 | `cpp_identifiers` | Seven valid/case-sensitive names | Seven identifier tokens | 9 checks; all identifiers recognized | PASS |
| 3 | `cpp_integer_literals` | `0 42 1000` | Three integer tokens | 8 checks; lexemes retained | PASS |
| 4 | `cpp_floating_literals` | `3.14 0.5 25.50 1.25f 2.0F` | Five floating tokens | 12 checks; suffixes retained | PASS |
| 5 | `cpp_character_literals` | Simple characters and documented escapes | Seven character tokens | 9 checks; all recognized | PASS |
| 6 | `cpp_string_literals` | Text and documented escapes | Seven string tokens | 9 checks; all recognized | PASS |
| 7 | `cpp_arithmetic_operators` | `+ - * / %` | Five arithmetic tokens | 7 checks; correct types | PASS |
| 8 | `cpp_relational_operators` | `< > <= >= == !=`, `x==1` | Correct longest-match tokens | 13 checks; `==` remained one token | PASS |
| 9 | `cpp_logical_operators` | `&& || !` | Three logical tokens | 5 checks; correct types | PASS |
| 10 | `cpp_assignment_operators` | `= += -= *= /=`, `x+=1` | Correct longest-match tokens | 12 checks; `+=` remained one token | PASS |
| 11 | `cpp_increment_decrement` | `x++ --y` | Identifier/increment/decrement sequence | 6 checks; correct sequence | PASS |
| 12 | `cpp_bitwise_operators` | `& | ^ ~ << >> -> . ::` | Nine operator tokens | 11 checks; shifts/scope recognized | PASS |
| 13 | `cpp_delimiters` | `( ) { } [ ] ; , :` | Nine delimiter tokens | 11 checks; correct types | PASS |
| 14 | `cpp_comments` | Line and multi-line comments around code | Comments discarded; seven tokens | 9 checks; expected tokens retained | PASS |
| 15 | `cpp_preprocessor` | Valid include/define, pragma, malformed directives | Three valid tokens and three errors | 16 checks; valid/invalid cases matched | PASS |
| 16 | `cpp_invalid_characters` | `int x; @ $` | Errors at columns 8 and 10 | 5 checks; two located errors, three valid tokens | PASS |
| 17 | `cpp_malformed_literals` | Bad numbers, literals, chars, escape, comment | Specific error for every case | 33 checks; all eleven cases diagnosed | PASS |
| 18 | `cpp_source_locations` | Two-line declaration/assignment fixture | Exact line/column sequence | 16 checks; all seven token locations matched | PASS |
| 19 | `cpp_cli_sample` | `examples/cpp-subset/01-full.cpp` | Token table, exit 0 | 48 tokens, including `std`, `::`, `cout`, `<<`, `endl`; exit 0 | PASS |
| 20 | `cpp_cli_invalid` | `examples/cpp-subset/02-lexical-errors.cpp` | Nonzero lexical-error exit | Four located diagnostics; exit 2 | PASS |
| 21 | `cpp_cli_help` | `optiq_cpp --help` | Usage text and exit 0 | Usage text found; exit 0 | PASS |

**Executed CTest result:** `100% tests passed, 0 tests failed out of 21`.

## Sample outputs

The complete generated token table is retained at
`examples/cpp-subset/01-full.tokens.txt`. The invalid sample emitted:

```text
Lexical error at line 2, column 17: Malformed numeric literal: 12.3.4
Lexical error at line 3, column 23: Invalid escape sequence: \q
Lexical error at line 4, column 24: Invalid character: @
Lexical error at line 5, column 1: Unterminated block comment
```

## Acceptance and limitations

All eighteen requested lexical categories and CLI behavior were executed. The
existing academic suite also passes. No Windows execution, parser, AST,
semantic analysis, preprocessing, TAC, optimization, or backend is claimed.
