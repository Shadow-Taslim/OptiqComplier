#pragma once

#include <memory>
#include <string>
#include <vector>

#include "optiq/cpp/ast.hpp"
#include "optiq/cpp/diagnostics.hpp"

namespace optiq::cpp {

struct ParseResult {
    std::unique_ptr<ProgramNode> program;
    std::vector<Diagnostic> lexicalDiagnostics;
    std::vector<Diagnostic> syntaxDiagnostics;

    bool success() const {
        return program != nullptr && lexicalDiagnostics.empty() && syntaxDiagnostics.empty();
    }
};

// Tokenizes and parses the documented C/C++ subset. Parsing stops before
// semantic analysis; no name resolution, type checking, or code generation occurs.
ParseResult parse(const std::string& source);

} // namespace optiq::cpp
