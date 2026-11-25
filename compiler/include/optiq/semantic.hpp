#pragma once

#include <string>
#include <vector>

#include "optiq/ast.hpp"

namespace optiq {

struct SemanticError {
    SourceLocation location;
    std::string message;
};

struct SemanticResult {
    std::vector<SemanticError> errors;

    bool success() const { return errors.empty(); }
};

// Walks the AST with a nested-scope symbol table and validates the rules the
// grammar alone cannot enforce: undeclared use, redeclaration, scope
// visibility, declaration/assignment type compatibility, operand types for
// arithmetic/relational/logical operators control conditions.
SemanticResult analyze(const Program& program);

} // namespace optiq
