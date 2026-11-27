# OptiQ Development Roadmap

## Product direction

The C/C++ common-subset compiler is OptiQ's primary product. The original
mini-language compiler remains independently buildable and testable as the
academic frontend and is frozen separately on `academic-baseline`.

## Week 1 — Foundation and architecture (completed)

- Audited the existing implementation and established its regression baseline.
- Defined the initial C/C++ lexical and language subset.
- Added an isolated C++17/Flex frontend and `optiq_cpp` executable.
- Preserved independent academic and C/C++ builds.
- Documented architecture, limitations, and verification.

## Week 2 — C/C++ lexical analyzer (completed)

- Recognized the documented keywords, identifiers, literals, operators,
  delimiters, comments, and limited preprocessor lines.
- Used longest-match tokenization and one-based line/column locations.
- Diagnosed malformed input without silently accepting unsupported syntax.
- Provided token-table CLI output and automated positive/negative tests.
- Re-ran the complete academic regression suite.

## Week 3 — C/C++ Bison parser and preliminary AST (completed in this working tree)

- Defined the supported context-free grammar and expression precedence ladder.
- Added a reentrant Bison C++ parser using the existing Flex token stream.
- Added a separate, extensible C/C++ AST with `std::unique_ptr` ownership and
  source locations.
- Implemented declarations, basic functions/calls, expressions, nested blocks,
  `if`/`else`, loops, returns, increment/decrement, stream I/O syntax, supported
  preprocessor nodes, and `using namespace std;`.
- Added located syntax diagnostics, semicolon-based statement recovery, and
  `--parse`/`--ast` CLI modes.
- Added positive, negative, structural AST, type-restriction, recovery, and CLI
  tests while retaining every Week 2 test.
- Re-ran both complete suites: 13/13 academic and 64/64 C/C++ tests passed.
- Documented grammar boundaries and parser architecture.

Week 3 completion applies to the locally tested working tree. Publishing the
changes requires separate user authorization.

## Week 4 — Semantic analysis (future; not implemented)

- Build C/C++ symbol tables and scope tracking from the preliminary AST.
- Check declarations, redeclarations, name use, function signatures, calls,
  return rules, and control-flow context where supported.
- Define operand, assignment, condition, stream, and conversion rules.
- Preserve syntax AST immutability where practical and add semantic diagnostics.
- Add valid/invalid semantic tests without changing the academic frontend.

## Week 5 — Intermediate representation (future; not implemented)

- Generate TAC or another documented IR only from semantically accepted ASTs.
- Define temporaries, labels, control flow, functions, calls, and stream lowering.
- Add IR verification and golden tests before considering optimization.

## Later phases (future; not implemented)

- Only after justification and specification: optimization and backend design.
- Object code, assembly, linking, runtime support, and executable generation are
  not implied by lexer/parser completion.

## Per-phase acceptance rule

A phase is complete only when its specification, implementation, examples, and
executed tests agree. Existing academic regressions must remain green. Future
work must not describe tokenization or parsing as evidence that a source program
is semantically valid or executable.
