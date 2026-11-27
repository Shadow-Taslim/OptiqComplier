#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "optiq/cpp/ast.hpp"
#include "optiq/cpp/diagnostics.hpp"
#include "optiq/cpp/token.hpp"

namespace optiq::cpp {

struct ParserContext {
    ParserContext(std::vector<Token> inputTokens, SourceLocation endLocation)
        : tokens(std::move(inputTokens)), eofLocation(endLocation) {}

    std::vector<Token> tokens;
    std::size_t nextToken = 0;
    SourceLocation eofLocation;
    std::unique_ptr<ProgramNode> program;
    std::vector<Diagnostic> diagnostics;

    void addSyntaxError(SourceLocation location, const std::string& message) {
        diagnostics.push_back(Diagnostic{message, location.line, location.column});
    }
};

SourceLocation computeEofLocation(const std::string& source);

} // namespace optiq::cpp
