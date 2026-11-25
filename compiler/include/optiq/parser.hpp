#pragma once

#include <memory>
#include <optional>
#include <string>

#include "optiq/ast.hpp"
#include "optiq/lexer.hpp"

namespace optiq {

struct ParseResult {
    std::unique_ptr<SelectStatement> statement;
    std::optional<Diagnostic> error;

    bool success() const { return statement != nullptr && !error.has_value(); }
};

struct ProgramParseResult {
    std::unique_ptr<Program> program;
    std::optional<Diagnostic> error;

    bool success() const { return program != nullptr && !error.has_value(); }
};

// Parses a single SQL SELECT statement (compatibility entry point).
ParseResult parseSql(const std::string& sql);

// Parses a full Optiq program per docs/language-spec.md.
ProgramParseResult parseProgram(const std::string& source);

} // namespace optiq
