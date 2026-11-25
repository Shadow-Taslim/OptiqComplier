#pragma once

#include <memory>
#include <string>
#include <vector>

#include "optiq/ast.hpp"
#include "optiq/lexer.hpp"

namespace optiq {

struct ParseResult {
    std::unique_ptr<Program> program;
    std::vector<Diagnostic> errors;

    bool success() const { return program != nullptr && errors.empty(); }
};

// Parses a full program in the course-specified language (.mc source).
// Uses Bison error recovery, so `errors` may contain several diagnostics.
ParseResult parseProgram(const std::string& source);

} // namespace optiq
