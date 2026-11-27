# OptiQ Development Roadmap

## Product direction

The C/C++ common-subset compiler is OptiQ's primary product. The original
mini-language compiler remains independently buildable and testable as the
academic frontend and is frozen separately on `academic-baseline`.

## Week 1 — Foundation and architecture

- Audit the existing implementation and establish its regression baseline.
- Define the initial C/C++ lexical and language subset.
- Add an isolated C++17/Flex frontend and `optiq_cpp` executable.
- Preserve independent academic and C/C++ builds.
- Document architecture, limitations, and verification.

## Week 2 — C/C++ lexical analyzer

- Recognize the documented keywords, identifiers, literals, operators,
  delimiters, comments, and limited preprocessor lines.
- Use longest-match tokenization and one-based line/column locations.
- Diagnose malformed input without silently accepting unsupported syntax.
- Provide token-table CLI output and automated positive/negative tests.
- Re-run the complete academic regression suite.

## Week 3 — Parser foundation (future; not implemented)

- Define the C/C++ subset context-free grammar and precedence rules.
- Add a separate Bison parser and C/C++ AST.
- Construct declarations, assignments, expressions, and blocks.
- Add syntax-error recovery and parser regression tests.

## Later semantic and intermediate phases (future; not implemented)

- C/C++ symbol tables, nested scopes, and type checking.
- Explicit semantic rules for operators, conditions, and conversions.
- Intermediate representation or TAC for the validated C/C++ AST.
- Only after justification and specification: optimization and backend design.

## Per-phase acceptance rule

A phase is complete only when its specification, implementation, examples, and
executed tests agree. Existing academic regressions must remain green. Future
work must not describe lexer tokens as evidence that a source program parses or
executes.
