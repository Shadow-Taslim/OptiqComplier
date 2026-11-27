#pragma once

#include <string>
#include <vector>

#include "optiq/cpp/diagnostics.hpp"
#include "optiq/cpp/token.hpp"

namespace optiq::cpp {

struct LexResult {
    std::vector<Token> tokens;
    std::vector<Diagnostic> diagnostics;

    bool success() const { return diagnostics.empty(); }
};

// Tokenizes only the explicitly documented C/C++ common subset. This function
// performs no preprocessing, parsing, semantic analysis, or code generation.
LexResult tokenize(const std::string& source);

} // namespace optiq::cpp
